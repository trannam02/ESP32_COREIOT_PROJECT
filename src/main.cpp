#define TINY_GSM_MODEM_SIM7600

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <Arduino_MQTT_Client.h>
#include <Server_Side_RPC.h>
#include <ThingsBoard.h>
#include <DHT.h>
#include <Arduino.h>
#include <TinyGsmClient.h>
#include <HardwareSerial.h>



HardwareSerial SerialAT(2);



const char apn[]  = "m3-world";
const char user[] = "mms";
const char pass[] = "mms";

constexpr char WIFI_SSID[] = "v";
constexpr char WIFI_PASSWORD[] = "12345678";

constexpr char TOKEN[] = "u2vs5285hon1e8pe2l33";
constexpr char THINGSBOARD_SERVER[] = "app.coreiot.io";
constexpr uint16_t THINGSBOARD_PORT = 1883U;

constexpr uint32_t SERIAL_DEBUG_BAUD = 115200U;

constexpr uint16_t MAX_MESSAGE_SEND_SIZE = 256U;
constexpr uint16_t MAX_MESSAGE_RECEIVE_SIZE = 256U;

constexpr uint8_t MAX_RPC_SUBSCRIPTIONS = 10U;
constexpr uint8_t MAX_RPC_RESPONSE = 10U;

WiFiClient espClient;
TinyGsm modem(SerialAT);
TinyGsmClient client(modem);
Arduino_MQTT_Client mqttClient(client);
// Initialize used apis
Server_Side_RPC<MAX_RPC_SUBSCRIPTIONS, MAX_RPC_RESPONSE> rpc;
const std::array<IAPI_Implementation*, 1U> apis = {
    &rpc
};


ThingsBoard tb(mqttClient, MAX_MESSAGE_RECEIVE_SIZE, MAX_MESSAGE_SEND_SIZE, Default_Max_Stack_Size, apis);

DHT dht;

void InitWiFi();
void readDHT11();
bool reconnect();

bool modemConnected = false;

void setup() {
  Serial.begin(SERIAL_DEBUG_BAUD);
  delay(1000);
  dht.setup(19);
  // InitWiFi();
  SerialAT.begin(115200, SERIAL_8N1, 16, 17);
  delay(1000);
  Serial.println(F("Initializing modem..."));
  modem.restart();

  String modemInfo = modem.getModemInfo();
  Serial.print(F("Modem: "));
  Serial.println(modemInfo);
}

void loop() {
  readDHT11();
  delay(1000);

  if (!modemConnected) {
    Serial.print(F("Waiting for network..."));
    if (!modem.waitForNetwork()) {
        Serial.println(" fail");
        delay(10000);
        return;
    }
    Serial.println(" OK");

    Serial.print(F("Connecting to "));
    Serial.print(apn);
    if (!modem.gprsConnect(apn, user, pass)) {
        Serial.println(" fail");
        delay(10000);
        return;
    }

    modemConnected = true;
    Serial.println(" OK");
  }

  if (!tb.connected()) {
    Serial.printf("Connecting to: (%s) with token (%s)\n", THINGSBOARD_SERVER, TOKEN);
    if (!tb.connect(THINGSBOARD_SERVER, TOKEN, THINGSBOARD_PORT)) {
      Serial.println("Failed to connect");
      return;
    }
  }
  tb.loop();
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
  tb.sendTelemetryData("temperature", 1);
  tb.sendTelemetryData("humidity", 2);
  Serial.printf("Temperature: %f --", 1);
  Serial.printf("Humidity %f\n", 2);
}
bool reconnect() {
  const wl_status_t status = WiFi.status();
  if (status == WL_CONNECTED) {
    return true;
  }
  InitWiFi();
  return true;
};