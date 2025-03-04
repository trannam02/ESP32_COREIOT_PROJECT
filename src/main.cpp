#include <WiFi.h>
#include <WiFiClientSecure.h>

#include <Arduino_MQTT_Client.h>
#include <Server_Side_RPC.h>
#include <ThingsBoard.h>
#include <DHT20.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

int r = 0;
int g = 0;
int b = 0;

constexpr char WIFI_SSID[] = "v";
constexpr char WIFI_PASSWORD[] = "12345678";

constexpr char TOKEN[] = "s6781q0kuqz4i7xvc3li";
constexpr char THINGSBOARD_SERVER[] = "app.coreiot.io";
constexpr uint16_t THINGSBOARD_PORT = 1883U;
constexpr uint32_t SERIAL_DEBUG_BAUD = 115200U;

// DEFINE CONST
#define NEOPIXEL GPIO_NUM_45
#define SDA GPIO_NUM_11
#define SCL GPIO_NUM_12

// Maximum size packets will ever be sent or received by the underlying MQTT client,
// if the size is to small messages might not be sent or received messages will be discarded
constexpr uint16_t MAX_MESSAGE_SEND_SIZE = 256U;
constexpr uint16_t MAX_MESSAGE_RECEIVE_SIZE = 256U;

constexpr const char RPC_JSON_METHOD[] = "example_json";
constexpr const char RPC_TEMPERATURE_METHOD[] = "example_set_temperature";
constexpr const char RPC_SWITCH_METHOD[] = "example_set_switch";
constexpr const char RPC_TEMPERATURE_KEY[] = "temp";
constexpr const char RPC_SWITCH_KEY[] = "switch";

constexpr const char RPC_NEO_RED_METHOD[] = "set_neo_red";
constexpr const char RPC_NEO_GREEN_METHOD[] = "set_neo_green";
constexpr const char RPC_NEO_BLUE_METHOD[] = "set_neo_blue";

constexpr const char RPC_NEO_RED_KEY[] = "data";
constexpr const char RPC_NEO_GREEN_KEY[] = "data";
constexpr const char RPC_NEO_BLUE_KEY[] = "data";

constexpr uint8_t MAX_RPC_SUBSCRIPTIONS = 10U;
constexpr uint8_t MAX_RPC_RESPONSE = 10U;

WiFiClient espClient;

// Initalize the Mqtt client instance
Arduino_MQTT_Client mqttClient(espClient);
// Initialize used apis
Server_Side_RPC<MAX_RPC_SUBSCRIPTIONS, MAX_RPC_RESPONSE> rpc;
const std::array<IAPI_Implementation*, 1U> apis = {
    &rpc
};
// Initialize ThingsBoard instance with the maximum needed buffer size
ThingsBoard tb(mqttClient, MAX_MESSAGE_RECEIVE_SIZE, MAX_MESSAGE_SEND_SIZE, Default_Max_Stack_Size, apis);

// Statuses for subscribing to rpc
bool subscribed = false;

void InitWiFi() {
  Serial.println("Connecting to AP ...");
  // Attempting to establish a connection to the given WiFi network
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected to AP");
}

bool reconnect() {
  // Check to ensure we aren't connected yet
  const wl_status_t status = WiFi.status();
  if (status == WL_CONNECTED) {
    return true;
  }

  // If we aren't establish a new connection to the given WiFi network
  InitWiFi();
  return true;
}

/// @brief Processes function for RPC call "example_json"
/// JsonVariantConst is a JSON variant, that can be queried using operator[]
/// See https://arduinojson.org/v5/api/jsonvariant/subscript/ for more details
/// @param data Data containing the rpc data that was called and its current value
/// @param response Data containgin the response value, any number, string or json, that should be sent to the cloud. Useful for getMethods
void processGetJson(const JsonVariantConst &data, JsonDocument &response) {
  Serial.println("Received the json RPC method");

  // Size of the response document needs to be configured to the size of the innerDoc + 1.
  StaticJsonDocument<JSON_OBJECT_SIZE(4)> innerDoc;
  innerDoc["string"] = "exampleResponseString";
  innerDoc["int"] = 5;
  innerDoc["float"] = 5.0f;
  innerDoc["bool"] = true;
  response["json_data"] = innerDoc;
}

void processTemperatureChange(const JsonVariantConst &data, JsonDocument &response) {
  Serial.println("Received the set temperature RPC method");

  // Process data
  const float example_temperature = data[RPC_TEMPERATURE_KEY];

  Serial.print("Example temperature: ");
  Serial.println(example_temperature);

  // Ensure to only pass values do not store by copy, or if they do increase the MaxRPC template parameter accordingly to ensure that the value can be deserialized.RPC_Callback.
  // See https://arduinojson.org/v6/api/jsondocument/add/ for more information on which variables cause a copy to be created
  response["string"] = "exampleResponseString";
  response["int"] = 5;
  response["float"] = 5.0f;
  response["double"] = 10.0;
  response["bool"] = true;
}

void processSwitchChange(const JsonVariantConst &data, JsonDocument &response) {
  Serial.println("Received the set switch method");

  // Process data
  const int switch_state = data[RPC_TEMPERATURE_KEY];
  Serial.print("Example switch state: ");
  Serial.println(switch_state);
  neopixelWrite(NEOPIXEL, switch_state, switch_state, switch_state);
  response.set(22.02);
}

void processNeoRed(const JsonVariantConst &data, JsonDocument &response){
  r = data[RPC_NEO_RED_KEY];
  Serial.print("Example switch state: ");
  Serial.println(r);
  neopixelWrite(NEOPIXEL, r, g, b);
  response.set(22.02);
}
void processNeoGreen(const JsonVariantConst &data, JsonDocument &response){
  g = data[RPC_NEO_GREEN_KEY];
  Serial.print("Example switch state: ");
  Serial.println(g);
  neopixelWrite(NEOPIXEL, r, g, b);
  response.set(22.02);
}
void processNeoBlue(const JsonVariantConst &data, JsonDocument &response){
  b = data[RPC_NEO_BLUE_KEY];
  Serial.print("Example switch state: ");
  Serial.println(b);
  neopixelWrite(NEOPIXEL, r, g, b);
  response.set(22.02);
}
void processNeoDefault(const JsonVariantConst &data, JsonDocument &response){
  response.set(0);
}


DHT20 DHT(&Wire);
void readDHT20(){
  int status = DHT.read();
  tb.sendTelemetryData("temperature", DHT.getTemperature());
  tb.sendTelemetryData("humidity", DHT.getHumidity());
}
void setup() {
  // Initalize serial connection for debugging
  Serial.begin(SERIAL_DEBUG_BAUD);
  delay(1000);
  InitWiFi();
  Wire.begin(SDA, SCL);
}


void loop() {
  readDHT20();
  
  delay(1000);
  if (!reconnect()) {
    return;
  }

  if (!tb.connected()) {
    // Reconnect to the ThingsBoard server,
    // if a connection was disrupted or has not yet been established
    Serial.printf("Connecting to: (%s) with token (%s)\n", THINGSBOARD_SERVER, TOKEN);
    if (!tb.connect(THINGSBOARD_SERVER, TOKEN, THINGSBOARD_PORT)) {
      Serial.println("Failed to connect");
      return;
    }
  }

  if (!subscribed) {
    Serial.println("Subscribing for RPC...");
    const std::array<RPC_Callback, MAX_RPC_SUBSCRIPTIONS> callbacks = {
      // Requires additional memory in the JsonDocument for the JsonDocument that will be copied into the response
      RPC_Callback{ RPC_JSON_METHOD,           processGetJson },
      // Requires additional memory in the JsonDocument for 5 key-value pairs that do not copy their value into the JsonDocument itself
      RPC_Callback{ RPC_TEMPERATURE_METHOD,    processTemperatureChange},
       // Internal size can be 0, because if we use the JsonDocument as a JsonVariant and then set the value we do not require additional memory
      RPC_Callback{ RPC_SWITCH_METHOD,         processSwitchChange},
      RPC_Callback{ RPC_NEO_RED_METHOD,         processNeoRed},
      RPC_Callback{ RPC_NEO_GREEN_METHOD,         processNeoGreen},
      RPC_Callback{ RPC_NEO_BLUE_METHOD,         processNeoBlue}
    };
    if (!rpc.RPC_Subscribe(callbacks.cbegin(), callbacks.cend())) {
      Serial.println("Failed to subscribe for RPC");
      return;
    }

    Serial.println("Subscribe done");
    subscribed = true;
  }

  tb.loop();
}


// // Task handles
// TaskHandle_t task1Handle = NULL;
// TaskHandle_t task2Handle = NULL;

// // Task functions
// void task1(void *pvParameters) {
//   while (1) {
//     Serial.println("Task 1 running...");
//     vTaskDelay(pdMS_TO_TICKS(1000)); // Delay for 1 second
//   }
// }

// void task2(void *pvParameters) {
//   while (1) {
//     Serial.println("Task 2 running...");
//     vTaskDelay(pdMS_TO_TICKS(2000)); // Delay for 2 seconds
//   }
// }

// void setup() {
//   Serial.begin(115200);

//   // Create tasks
//   xTaskCreate(
//       task1,        // Task function
//       "Task 1",     // Task name
//       10000,        // Stack size (bytes)
//       NULL,         // Task parameters
//       1,            // Priority (lower number = lower priority)
//       &task1Handle); // Task handle

//   xTaskCreate(
//       task2,        // Task function
//       "Task 2",     // Task name
//       10000,        // Stack size (bytes)
//       NULL,         // Task parameters
//       2,            // Priority (higher priority than task1)
//       &task2Handle); // Task handle
// }

// void loop() {
//   // Empty loop, tasks are managed by FreeRTOS
// }