#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <Arduino_MQTT_Client.h>
#include <Server_Side_RPC.h>
#include <ThingsBoard.h>
#include <DHT.h>

#define SDA_PIN GPIO_NUM_11
#define SCL_PIN GPIO_NUM_12

///====================================================== CONST
constexpr char WIFI_SSID[] = "v";
constexpr char WIFI_PASSWORD[] = "12345678";

constexpr char TOKEN[] = "s6781q0kuqz4i7xvc3li";
constexpr char THINGSBOARD_SERVER[] = "app.coreiot.io";
constexpr uint16_t THINGSBOARD_PORT = 1883U;

constexpr uint32_t SERIAL_DEBUG_BAUD = 115200U;

constexpr uint16_t MAX_MESSAGE_SEND_SIZE = 256U;
constexpr uint16_t MAX_MESSAGE_RECEIVE_SIZE = 256U;

constexpr uint8_t MAX_RPC_SUBSCRIPTIONS = 10U;
constexpr uint8_t MAX_RPC_RESPONSE = 10U;

WiFiClient espClient;
Arduino_MQTT_Client mqttClient(espClient);
DHT dht;

// Initialize used apis
Server_Side_RPC<MAX_RPC_SUBSCRIPTIONS, MAX_RPC_RESPONSE> rpc;
const std::array<IAPI_Implementation*, 1U> apis = {
    &rpc
};
ThingsBoard tb(mqttClient, MAX_MESSAGE_RECEIVE_SIZE, MAX_MESSAGE_SEND_SIZE, Default_Max_Stack_Size, apis);

void InitWiFi();
void readDHT11();
bool reconnect();

void setup() {
  Serial.begin(SERIAL_DEBUG_BAUD);
  delay(1000);
  // InitWiFi();
  dht.setup(6);
}

void loop() {
  // static unsigned long lastDHTReadTime = 0;
  // unsigned long currentMillis = millis();

  // Run readDHT20 every 1 second without blocking delay()
  // if (currentMillis - lastDHTReadTime >= 1000) {
  //   readDHT20();
  //   lastDHTReadTime = currentMillis;
  // }
  readDHT11();
  delay(1000);
  // if (!reconnect()) {
  //   return;
  // }

  // if (!tb.connected()) {
  //   Serial.printf("Connecting to: (%s) with token (%s)\n", THINGSBOARD_SERVER, TOKEN);
  //   if (!tb.connect(THINGSBOARD_SERVER, TOKEN, THINGSBOARD_PORT)) {
  //     Serial.println("Failed to connect");
  //     return;
  //   }
  // }
  // tb.loop();
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
  float temp = dht.getTemperature();
  float humi = dht.getHumidity();
  Serial.printf("Temperature: %f", temp);
  Serial.printf(" - Humidity: %f", humi);
  tb.sendTelemetryData("temperature", temp);
  tb.sendTelemetryData("humidity", humi);
}
bool reconnect() {
  const wl_status_t status = WiFi.status();
  if (status == WL_CONNECTED) {
    return true;
  }
  InitWiFi();
  return true;
};