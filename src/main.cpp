#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <Arduino_MQTT_Client.h>
#include <Server_Side_RPC.h>
#include <ThingsBoard.h>

#include <Arduino.h>
#include <Attribute_Request.h>
#include <Shared_Attribute_Update.h>
#include "./utils/env.h"
#include <OTA_Firmware_Update.h>
#include <Espressif_Updater.h>

int led_state = 0;
// ======================================================== OBJECT
WiFiClient espClient;
Arduino_MQTT_Client mqttClient(espClient);

// Initialize used apis
constexpr std::array<const char *, 2U> SHARED_ATTRIBUTES_LIST = {
  "ledstate",
  "fw_version"
};

// List of client attributes for requesting them (Using to initialize device states)
constexpr std::array<const char *, 2U> CLIENT_ATTRIBUTES_LIST = {
  "ledmode",
  "ledstate"
};
Server_Side_RPC<MAX_RPC_SUBSCRIPTIONS, MAX_RPC_RESPONSE> rpc;
Attribute_Request<2U, MAX_ATTRIBUTES> attr_request;
Shared_Attribute_Update<3U, MAX_ATTRIBUTES> shared_update;
OTA_Firmware_Update<> ota;
Espressif_Updater<> updater;

const std::array<IAPI_Implementation*, 4U> apis = {
    &rpc,
    &attr_request,
    &shared_update,
    &ota
};

ThingsBoard tb(mqttClient, MAX_MESSAGE_RECEIVE_SIZE, MAX_MESSAGE_SEND_SIZE, Default_Max_Stack_Size, apis);

bool myLED = 0;
int light_count = 5;
const char KEY_myLED[] = "ledstate";

bool subscribed = false;

bool currentFWSent = false;
bool updateRequestSent = false;

// ========================== FUNCTION PROTOTYPES
void InitWiFi();
void readDHT11();
bool reconnect();
void processClientAttributes(const JsonObjectConst &data);
void processSharedAttributes(const JsonObjectConst &data);
void processSharedAttributesQQ(const JsonObjectConst &data);
void requestTimedOut();

const Shared_Attribute_Callback<MAX_ATTRIBUTES> attributes_callback(&processSharedAttributes, SHARED_ATTRIBUTES_LIST.cbegin(), SHARED_ATTRIBUTES_LIST.cend());
const Attribute_Request_Callback<MAX_ATTRIBUTES> attribute_shared_request_callback(&processSharedAttributesQQ, 10000, &requestTimedOut, SHARED_ATTRIBUTES_LIST);
const Attribute_Request_Callback<MAX_ATTRIBUTES> attribute_client_request_callback(&processClientAttributes, 10000, &requestTimedOut, CLIENT_ATTRIBUTES_LIST);

// =========================== TASKS
TaskHandle_t turnLedTaskHandle = NULL; // Handle for task_blinky

void task_blinky(void *pvParameters);
void init_attribute();
void task_connect(void *pvParameters);
void task_firmware_update(void *pvParameters);

void update_starting_callback() {
  Serial.println("Starting firmware update...");
  if (turnLedTaskHandle != NULL) {
    vTaskDelete(turnLedTaskHandle);
    turnLedTaskHandle = NULL;
    Serial.println("Stopped task_blinky");
  }
}

void finished_callback(const bool & success) {
  if (success) {
    const esp_partition_t *update_partition = esp_ota_get_next_update_partition(NULL);
    if (update_partition != NULL) {
      esp_err_t err = esp_ota_set_boot_partition(update_partition);
      if (err == ESP_OK) {
        Serial.println("Boot partition set successfully. Rebooting...");
        esp_restart();
      } else {
        Serial.printf("Failed to set boot partition: %s\n", esp_err_to_name(err));
      }
    } else {
      Serial.println("No valid update partition found.");
    }
    Serial.println("Done, Reboot now");
    return;
  }
  Serial.println("Downloading firmware failed");
}

void progress_callback(const size_t & current, const size_t & total) {
  Serial.printf("Progress %.2f%%\n", static_cast<float>(current * 100U) / total);
}
void setup() {
  Serial.begin(SERIAL_DEBUG_BAUD);
  // dht.setup(19);
  InitWiFi();
  xTaskCreate(task_blinky, "Read DHT20", 4096, NULL, 1, &turnLedTaskHandle);
  xTaskCreate(task_connect, "Task connect to thingsboard", 4096, NULL, 2, NULL);
  xTaskCreate(task_firmware_update, "Request Firmware Update", 4096, NULL, 3, NULL);
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

void requestTimedOut() {
  Serial.print("Request Timeout!.!.!");
}
void processClientAttributes(const JsonObjectConst &data) {
  for (auto it = data.begin(); it != data.end(); ++it) {
    Serial.printf("Client atrribute: %s", it->key().c_str());
  }
}
void processSharedAttributes(const JsonObjectConst &data) {
  Serial.print("Shared attribute changed!!\n");
  for (auto it = data.begin(); it != data.end(); ++it) {
    if(strcmp(it->key().c_str(), KEY_myLED) == 0){
      myLED = it->value().as<bool>();
      light_count = 5;
    };
    if(strcmp(it->key().c_str(), "fw_version") == 0){
      currentFWSent = false;
      updateRequestSent = false;
    };
  }
} 
void processSharedAttributesQQ(const JsonObjectConst &data) {
  for (auto it = data.begin(); it != data.end(); ++it) {
    Serial.printf("Request atrribute: %s", it->key().c_str());
  }
}

void init_attribute(){
  Serial.print("Init Attribute...\n");
  if (!attr_request.Shared_Attributes_Request(attribute_shared_request_callback)) {
    Serial.println("Failed to request for shared attributes");
    return;
  };
  if (!attr_request.Client_Attributes_Request(attribute_client_request_callback)) {
    Serial.println("Failed to request for client attributes");
    return;
  };
  Serial.print("Init Attribute OK...\n");
}  
void task_blinky(void *pvParameters) {
  while (1) {
    Serial.print("["); Serial.print(CURRENT_FIRMWARE_VERSION); Serial.print("] LED is blinking at frequency: "); Serial.print(blinking_frequency); Serial.println(" Hz");
    digitalWrite(GPIO_NUM_2, 1);
    if (myLED == 0)
      myLED = 1;
    else
      myLED = 0;
    vTaskDelay(pdMS_TO_TICKS(200));
  }
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
      if (!shared_update.Shared_Attributes_Subscribe(attributes_callback)) {
        Serial.println("Failed to subscribe for shared attribute updates");
        return;
      }
      init_attribute();
      subscribed = true;
    }
    tb.loop();
    vTaskDelay(pdMS_TO_TICKS(200));
  }
}
void task_firmware_update(void *pvParameters){
  while(1){
    if(tb.connected()){
      if (!currentFWSent) {
        currentFWSent = ota.Firmware_Send_Info(CURRENT_FIRMWARE_TITLE, CURRENT_FIRMWARE_VERSION);
      }
      if (!updateRequestSent) {
        Serial.print("Firwmare Update Request Sent...\n");
        const OTA_Update_Callback callback(CURRENT_FIRMWARE_TITLE, CURRENT_FIRMWARE_VERSION, &updater, &finished_callback, &progress_callback, &update_starting_callback, FIRMWARE_FAILURE_RETRIES, FIRMWARE_PACKET_SIZE);
        updateRequestSent = ota.Start_Firmware_Update(callback);
      }
    }
    vTaskDelay(pdMS_TO_TICKS(10000));
  }
};
