#include <Arduino.h>
#include <Espressif_Updater.h>
#include <Arduino_MQTT_Client.h>
#include <OTA_Firmware_Update.h>
#include <ThingsBoard.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

// ========================= CONNECTION
constexpr char WIFI_SSID[] = "v";
constexpr char WIFI_PASSWORD[] = "12345678";
constexpr char TOKEN[] = "wp5G2SxfWCXWgzL9nwyE";
constexpr char THINGSBOARD_SERVER[] = "app.coreiot.io";
constexpr uint16_t THINGSBOARD_PORT = 1883U;

// ========================= DEBUG
constexpr uint32_t SERIAL_DEBUG_BAUD = 115200U;

// ========================= SIZE
constexpr uint16_t MAX_MESSAGE_SEND_SIZE = 256U;
constexpr uint16_t MAX_MESSAGE_RECEIVE_SIZE = 256U;

constexpr uint8_t MAX_RPC_SUBSCRIPTIONS = 10U;
constexpr uint8_t MAX_RPC_RESPONSE = 10U;
constexpr uint8_t MAX_ATTRIBUTES = 10U;


constexpr char CURRENT_FIRMWARE_TITLE[] = "blinky_firmware_ota";
constexpr char CURRENT_FIRMWARE_VERSION[] = "2.0.0";

constexpr uint8_t FIRMWARE_FAILURE_RETRIES = 12U;
constexpr uint16_t FIRMWARE_PACKET_SIZE = 4096U;

// MY VARIABLES
int blinking_frequency = 5;