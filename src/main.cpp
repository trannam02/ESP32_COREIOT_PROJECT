#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <Arduino_MQTT_Client.h>
#include <Server_Side_RPC.h>
#include <ThingsBoard.h>
#include <DHT.h>
#include <Arduino.h>
#include <Attribute_Request.h>
#include <Shared_Attribute_Update.h>
#include "./utils/env.h"

// ========================== OBJECT
WiFiClient espClient;
Arduino_MQTT_Client mqttClient(espClient);
DHT dht;



// Initialize used apis
constexpr std::array<const char *, 2U> SHARED_ATTRIBUTES_LIST = {
  "ledstate"
};

// List of client attributes for requesting them (Using to initialize device states)
constexpr std::array<const char *, 2U> CLIENT_ATTRIBUTES_LIST = {
  "ledmode",
  "ledstate"
};
Server_Side_RPC<MAX_RPC_SUBSCRIPTIONS, MAX_RPC_RESPONSE> rpc;
Attribute_Request<2U, MAX_ATTRIBUTES> attr_request;
Shared_Attribute_Update<3U, MAX_ATTRIBUTES> shared_update;

const std::array<IAPI_Implementation*, 3U> apis = {
    &rpc,
    &attr_request,
    &shared_update
};

ThingsBoard tb(mqttClient, MAX_MESSAGE_RECEIVE_SIZE, MAX_MESSAGE_SEND_SIZE, Default_Max_Stack_Size, apis);


void InitWiFi();
void readDHT11();
bool reconnect();

bool myLED = 0;
int light_count = 5;
const char KEY_myLED[] = "ledstate";

bool subscribed = false;
bool attributesChanged = false;

void processSharedAttributes(const JsonObjectConst &data) {
  for (auto it = data.begin(); it != data.end(); ++it) {
    if(strcmp(it->key().c_str(), KEY_myLED) == 0){
      myLED = it->value().as<bool>();
      // Serial.printf("Receive Share Attribute: %d\n", myLED);
      light_count = 5;
    };
  }
  
  attributesChanged = true;
} 
void processSharedAttributesQQ(const JsonObjectConst &data) {
  for (auto it = data.begin(); it != data.end(); ++it) {
    Serial.printf("Requesr atrribute: %s", it->key().c_str());
  }
  Serial.printf("REQUEST");
  // Change some info
  attributesChanged = true;
}
void processClientAttributes(const JsonObjectConst &data) {
  for (auto it = data.begin(); it != data.end(); ++it) {
    Serial.printf("Client atrribute: %s", it->key().c_str());
  }
}

void requestTimedOut() {
  Serial.printf("Request Timeout!.!.!");
}

const Shared_Attribute_Callback<MAX_ATTRIBUTES> attributes_callback(&processSharedAttributes, SHARED_ATTRIBUTES_LIST.cbegin(), SHARED_ATTRIBUTES_LIST.cend());
const Attribute_Request_Callback<MAX_ATTRIBUTES> attribute_shared_request_callback(&processSharedAttributesQQ, 10000, &requestTimedOut, SHARED_ATTRIBUTES_LIST);
const Attribute_Request_Callback<MAX_ATTRIBUTES> attribute_client_request_callback(&processClientAttributes, 10000, &requestTimedOut, CLIENT_ATTRIBUTES_LIST);

void processNeon(const JsonVariantConst &data, JsonDocument &response){
  const int switch_state = data["key"];
  Serial.printf("Example switch state: %d", switch_state);
};
void task_turn_led_on_5s(void *pvParameters) {
  while (1) {
    Serial.printf("LED is: %s\n", (myLED ? "ON" : "OFF"));
    int led_state = myLED ? 255 : 0;
    neopixelWrite(45, led_state, led_state, led_state);
    light_count -= 1;
    if(light_count <= 0){
      myLED = 0;
    };
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}
void task_init_attribute(){
  Serial.printf("Init Attribute...\n");
  if (!attr_request.Shared_Attributes_Request(attribute_shared_request_callback)) {
    Serial.println("Failed to request for shared attributes");
    return;
  };
  if (!attr_request.Client_Attributes_Request(attribute_client_request_callback)) {
    Serial.println("Failed to request for client attributes");
    return;
  };
  Serial.printf("Init Attribute OK...\n");
}

void task_connect(void *pvParameters) {
  while (1) {
    if (!reconnect()) {
      return;
    }
    if (!tb.connected()) {
      Serial.printf("Connecting to: (%s) with token (%s)\n", THINGSBOARD_SERVER, TOKEN);
      if (!tb.connect(THINGSBOARD_SERVER, TOKEN, THINGSBOARD_PORT)) {
        Serial.println("Failed to connect");
        return;
      }
    };

    if (!subscribed) {
      const std::array<RPC_Callback, MAX_RPC_SUBSCRIPTIONS> callbacks = {
        RPC_Callback{ "key", processNeon }
      };
      if (!rpc.RPC_Subscribe(callbacks.cbegin(), callbacks.cend())) {
        Serial.println("Failed to subscribe for RPC");
        return;
      }
      Serial.println("Subscribe RPC: OK");
      if (!shared_update.Shared_Attributes_Subscribe(attributes_callback)) {
        Serial.println("Failed to subscribe for shared attribute updates");
        return;
      }
      Serial.println("Subscribe for shared attribute updates: OK");
      task_init_attribute();
      subscribed = true;
    }
    tb.loop();
    vTaskDelay(pdMS_TO_TICKS(200));
  }
}

void setup() {
  Serial.begin(SERIAL_DEBUG_BAUD);
  dht.setup(19);
  InitWiFi();
  
  
  xTaskCreate(task_connect, "My Connection Establish", 4096, NULL, 2, NULL);
  xTaskCreate(task_turn_led_on_5s, "Read DHT20", 4096, NULL, 1, NULL);
  
}

void loop() {

}

void InitWiFi() {
  Serial.println("Connecting to AP ...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected to AP");
}
void readDHT11(){
  // int status = DHT.read();
  float temp = 1;
  float humi = 2;
  // float temp = dht.getTemperature();
  // float humi = dht.getHumidity();
  // tb.sendTelemetryData("temperature", dht.getTemperature());
  // tb.sendTelemetryData("humidity", dht.getHumidity());
  tb.sendTelemetryData("temperature", temp);
  tb.sendTelemetryData("humidity", humi);
  Serial.printf("Temperature: %f --", temp);
  Serial.printf("Humidity %f\n", humi);
}
bool reconnect() {
  const wl_status_t status = WiFi.status();
  if (status == WL_CONNECTED) {
    return true;
  }
  InitWiFi();
  return true;
};