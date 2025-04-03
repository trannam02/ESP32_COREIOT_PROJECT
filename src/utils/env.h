#include <Arduino.h>

// ========================= CONNECTION
constexpr char WIFI_SSID[] = "v";
constexpr char WIFI_PASSWORD[] = "12345678";
constexpr char TOKEN[] = "u2vs5285hon1e8pe2l33";
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