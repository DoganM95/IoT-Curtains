// Configurations
#include "./Configuration/Blynk.h"
#include "./Configuration/Wifi.h"

// Libraries
#include <BlynkSimpleEsp32.h>  // Part of Blynk by Volodymyr Shymanskyy
#include <WiFi.h>              // Part of WiFi Built-In by Arduino
#include <WiFiClient.h>        // Part of WiFi Built-In by Arduino
#include <math.h>

#include <future>

// Configurations
const u_int pressDuration = 500;  // time the gpio should be HIGH, before it goes back to LOW

// GPIO pins
const u_short openCurtainsPin = 26;   // Put a pulldown resistor on this one (10 kOhm from gpio -> GND)
const u_short closeCurtainsPin = 25;  // Put a pulldown resistor on this one (10 kOhm from gpio -> GND)

const u_short openCurtainsPhysicalButtonPin = 34;
const u_short closeCurtainsPhysicalButtonPin = 35;

const u_short ledRedPin = 14;
const u_short ledGreenPin = 12;
const u_short ledBluePin = 27;  // Workaround/fix for current iteration: jmp R5, omit R6 and R7

// Limits
const int wifiHandlerThreadStackSize = 10000;
const int blynkHandlerThreadStackSize = 10000;

// Counters
unsigned long long wifiReconnectCounter = 0;
unsigned long long blynkReconnectCounter = 0;

// Timeouts
int wifiConnectionTimeout = 10000;
int blynkConnectionTimeout = 10000;
int blynkConnectionStabilizerTimeout = 5000;
ushort cycleDelayInMilliSeconds = 100;

// Task Handles
TaskHandle_t wifiConnectionHandlerThreadFunctionHandle;
TaskHandle_t blynkConnectionHandlerThreadFunctionHandle;
TaskHandle_t buttonSensorThreadFunctionHandle;

// Declarations
void blynkConnectionHandlerThreadFunction(void* params);
void wifiConnectionHandlerThreadFunction(void* params);
void buttonSensorThread(void* params);
void pressButton(u_short pin, u_int durationInMs);

// Led Colors
class led {
 public:
  inline static std::array<int, 3> black = {0, 0, 0};
  inline static std::array<int, 3> red = {1, 0, 0};
  inline static std::array<int, 3> green = {0, 1, 0};
  inline static std::array<int, 3> blue = {0, 0, 1};
  inline static std::array<int, 3> yellow = {1, 1, 0};
  inline static std::array<int, 3> magenta = {1, 0, 1};
  inline static std::array<int, 3> cyan = {0, 1, 1};
  inline static std::array<int, 3> white = {1, 1, 1};

  static std::future<void> setColorAsync(const std::array<int, 3>& color, u_int durationInMs) {
    return std::async(std::launch::async, [=]() {
      analogWrite(ledRedPin, color[0] * 8);
      analogWrite(ledGreenPin, color[1] * 8);
      analogWrite(ledBluePin, color[2] * 8);
      delay(durationInMs); // becomes irrelevant when calling async, is used in sync ( .get() )
    });
  }
};

// SETUP
void setup() {
  Serial.begin(115200);

  pinMode(openCurtainsPin, OUTPUT);
  pinMode(closeCurtainsPin, OUTPUT);

  pinMode(openCurtainsPhysicalButtonPin, INPUT);
  pinMode(closeCurtainsPhysicalButtonPin, INPUT);

  pinMode(ledRedPin, OUTPUT);
  pinMode(ledGreenPin, OUTPUT);
  pinMode(ledBluePin, OUTPUT);

  led::setColorAsync(led::red, 1000).get();
  led::setColorAsync(led::green, 1000).get();
  led::setColorAsync(led::blue, 1000).get();

  xTaskCreatePinnedToCore(wifiConnectionHandlerThreadFunction, "Wifi Connection Handling Thread", wifiHandlerThreadStackSize, NULL, 20, &wifiConnectionHandlerThreadFunctionHandle, 1);
  xTaskCreatePinnedToCore(blynkConnectionHandlerThreadFunction, "Blynk Connection Handling Thread", blynkHandlerThreadStackSize, NULL, 20, &blynkConnectionHandlerThreadFunctionHandle, 1);
  // TODO: reactivate in bug-fixed pcb iteration, where button pin is connected to 3v3 instead of ground...
  // xTaskCreatePinnedToCore(buttonSensorThread, "Physical Button Sensing Thread", 10000, NULL, 20, &buttonSensorThreadFunctionHandle, 1);
}

// MAIN LOOP
void loop() { Blynk.run(); }

// FUNCTIONS

BLYNK_CONNECTED() {  // Restore hardware pins according to current UI config
  Blynk.syncAll();
}

BLYNK_WRITE(V1) {  // Open curtains
  int pinValue = param.asInt();
  pressButton(openCurtainsPin, pressDuration);
}

BLYNK_WRITE(V2) {  // Close curtains
  int pinValue = param.asInt();
  pressButton(closeCurtainsPin, pressDuration);
}

void buttonSensorThread(void* params) {
  while (true) {
    if (digitalRead(openCurtainsPhysicalButtonPin) == HIGH) {
      pressButton(openCurtainsPin, pressDuration);  // blocks this loop for pressDuration, serves as debounce
    }
    if (digitalRead(closeCurtainsPhysicalButtonPin) == HIGH) {
      pressButton(closeCurtainsPin, pressDuration);  // blocks this loop for pressDuration, serves as debounce
    }
    delay(10);  // Lessen cpu usage
  }
}

void pressButton(u_short pin, u_int durationInMs) {
  digitalWrite(pin, 1);
  delay(durationInMs);  // also serves as a debounce delay for the physical button
  digitalWrite(pin, 0);
  Serial.printf("Button %d pressed\n", pin);
}

void WaitForWifi(uint cycleDelayInMilliSeconds) {
  while (WiFi.status() != WL_CONNECTED) {
    delay(cycleDelayInMilliSeconds);
  }
}

void WaitForBlynk(int cycleDelayInMilliSeconds) {
  while (!Blynk.connected()) {
    delay(cycleDelayInMilliSeconds);
  }
}

void wifiConnectionHandlerThreadFunction(void* params) {
  uint time;
  while (true) {
    if (!WiFi.isConnected()) {
      try {
        Serial.printf("Connecting to Wifi: %s\n", WIFI_SSID);
        WiFi.begin(WIFI_SSID, WIFI_PW);  // initial begin as workaround to some espressif library bug
        WiFi.disconnect();
        WiFi.begin(WIFI_SSID, WIFI_PW);
        WiFi.setHostname("Desklight (ESP32, Blynk)");
        time = 0;
        while (WiFi.status() != WL_CONNECTED) {
          if (time >= wifiConnectionTimeout || WiFi.isConnected()) break;
          delay(cycleDelayInMilliSeconds);
          time += cycleDelayInMilliSeconds;
        }
      } catch (const std::exception e) {
        Serial.printf("Error occured: %s\n", e.what());
      }
      if (WiFi.isConnected()) {
        Serial.printf("Connected to Wifi: %s\n", WIFI_SSID);
        wifiReconnectCounter = 0;
      }
    }
    delay(1000);
    Serial.printf("Wifi Connection Handler Thread current stack size: %d , current Time: %d\n", wifiHandlerThreadStackSize - uxTaskGetStackHighWaterMark(NULL), xTaskGetTickCount());
  };
}

void blynkConnectionHandlerThreadFunction(void* params) {
  uint time;
  while (true) {
    if (!Blynk.connected()) {
      Serial.printf("Connecting to Blynk: %s\n", BLYNK_USE_LOCAL_SERVER == true ? BLYNK_SERVER : "Blynk Cloud Server");
      if (BLYNK_USE_LOCAL_SERVER)
        Blynk.config(BLYNK_AUTH, BLYNK_SERVER, BLYNK_PORT);
      else
        Blynk.config(BLYNK_AUTH);
      Blynk.connect();  // Connects using the chosen Blynk.config
      uint time = 0;
      while (!Blynk.connected()) {
        if (time >= blynkConnectionTimeout || Blynk.connected()) break;
        delay(cycleDelayInMilliSeconds);
        time += cycleDelayInMilliSeconds;
      }
      if (Blynk.connected()) {
        Serial.printf("Connected to Blynk: %s\n", BLYNK_USE_LOCAL_SERVER ? BLYNK_SERVER : "Blynk Cloud Server");
        delay(blynkConnectionStabilizerTimeout);
      }
    }
    delay(1000);
    Serial.printf("Blynk Connection Handler Thread current stack size: %d , current Time: %d\n", blynkHandlerThreadStackSize - uxTaskGetStackHighWaterMark(NULL), xTaskGetTickCount());
  }
}
