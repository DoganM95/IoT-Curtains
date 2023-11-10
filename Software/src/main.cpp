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
u_int ledBrightness = 4;          // between 0 and 255, respective to 0 to 100 %

// GPIO pins
const u_short openCurtainsPin = 26;   // Put a pulldown resistor on this one (10 kOhm from gpio -> GND)
const u_short closeCurtainsPin = 25;  // Put a pulldown resistor on this one (10 kOhm from gpio -> GND)

const u_short openCurtainsPhysicalButtonPin = 34;
const u_short closeCurtainsPhysicalButtonPin = 35;

const u_short ledRedPin = 14;
const u_short ledGreenPin = 12;
const u_short ledBluePin = 27;  // Workaround/fix for current iteration: jmp R5, omit R6 and R7

const u_short ledRedWallPin = 19;
const u_short ledGreenWallPin = 18;

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
TaskHandle_t redWallLedSensorThreadFunctionHandle;
TaskHandle_t greenWallLedSensorThreadFunctionHandle;
TaskHandle_t blueLedThreadFunctionHandle;

// Declarations
void blynkConnectionHandlerThreadFunction(void* params);
void wifiConnectionHandlerThreadFunction(void* params);
void buttonSensorThread(void* params);
void pressButton(u_short pin, u_int durationInMs);
void redWallLedSensorThread(void* params);
void greenWallLedSensorThread(void* params);
void blueLedThread(void* params);

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

  inline static std::array<int, 3> lastColor = {0, 0, 0};
  inline static std::array<int, 3> previousColor = {0, 0, 0};

  static std::future<void> setColorAsync(const std::array<int, 3>& color, u_int durationInMs) {
    return std::async(std::launch::async, [=]() {
      led::previousColor = led::lastColor;  // Save the last color before changing it
      led::lastColor = color;
      analogWrite(ledRedPin, color[0] * ledBrightness);
      analogWrite(ledGreenPin, color[1] * ledBrightness);
      analogWrite(ledBluePin, color[2] * ledBrightness);
      delay(durationInMs);
    });
  }

  static std::future<void> addColorAsync(const std::array<int, 3>& color, u_int durationInMs) {
    return std::async(std::launch::async, [=]() {
      const int ledPins[] = {ledRedPin, ledGreenPin, ledBluePin};
      for (int i = 0; i < 3; ++i) {
        if (color[i] == 1) {
          analogWrite(ledPins[i], color[i] * ledBrightness);
        }
      }
      delay(durationInMs);
    });
  }

  static std::future<void> removeColorAsync(const std::array<int, 3>& color) {
    return std::async(std::launch::async, [=]() {
      const int ledPins[] = {ledRedPin, ledGreenPin, ledBluePin};
      for (int i = 0; i < 3; ++i) {
        if (color[i] == 1) {
          analogWrite(ledPins[i], 0);
        }
      }
    });
  }

  static void setPreviousColor() {
    analogWrite(ledRedPin, previousColor[0] * ledBrightness);
    analogWrite(ledGreenPin, previousColor[1] * ledBrightness);
    analogWrite(ledBluePin, previousColor[2] * ledBrightness);
  }

  static void setBrightness(u_int brightness) {
    ledBrightness = brightness;
    led::setColorAsync(led::lastColor, 0).get();
  }
};

// SETUP
void setup() {
  Serial.begin(115200);

  pinMode(openCurtainsPin, OUTPUT);
  pinMode(closeCurtainsPin, OUTPUT);

  pinMode(openCurtainsPhysicalButtonPin, INPUT);
  pinMode(closeCurtainsPhysicalButtonPin, INPUT);

  pinMode(ledRedWallPin, INPUT_PULLDOWN);
  pinMode(ledGreenWallPin, INPUT_PULLDOWN);

  pinMode(ledRedPin, OUTPUT);
  pinMode(ledGreenPin, OUTPUT);
  pinMode(ledBluePin, OUTPUT);

  led::setColorAsync(led::red, 1000).get();
  led::setColorAsync(led::green, 1000).get();
  led::setColorAsync(led::blue, 1000).get();
  led::setColorAsync(led::black, 0).get();

  xTaskCreatePinnedToCore(wifiConnectionHandlerThreadFunction, "Wifi Connection Handling Thread", wifiHandlerThreadStackSize, NULL, 20, &wifiConnectionHandlerThreadFunctionHandle, 1);
  xTaskCreatePinnedToCore(blynkConnectionHandlerThreadFunction, "Blynk Connection Handling Thread", blynkHandlerThreadStackSize, NULL, 20, &blynkConnectionHandlerThreadFunctionHandle, 1);
  // TODO: reactivate in bug-fixed pcb iteration, where button pin is connected to 3v3 instead of ground...
  // xTaskCreatePinnedToCore(buttonSensorThread, "Physical Button Sensing Thread", 10000, NULL, 20, &buttonSensorThreadFunctionHandle, 0);
  xTaskCreatePinnedToCore(redWallLedSensorThread, "Red Wall Led Sensor Thread", 10000, NULL, 20, &redWallLedSensorThreadFunctionHandle, 0);
  xTaskCreatePinnedToCore(greenWallLedSensorThread, "Green Wall Led Sensor Thread", 10000, NULL, 20, &greenWallLedSensorThreadFunctionHandle, 0);
  xTaskCreatePinnedToCore(blueLedThread, "Blue Led Thread", 10000, NULL, 20, &blueLedThreadFunctionHandle, 0);
}

// MAIN LOOP
void loop() { Blynk.run(); }

// FUNCTIONS

BLYNK_CONNECTED() {  // Restore hardware pins according to current UI config
  Blynk.syncAll();
}

BLYNK_WRITE(V0) {  // Open curtains
  int pinValue = param.asInt();
  led::setBrightness(pinValue);
}

BLYNK_WRITE(V1) {  // Open curtains
  int pinValue = param.asInt();
  if (pinValue == 1) pressButton(openCurtainsPin, pressDuration);
}

BLYNK_WRITE(V2) {  // Set led brightness
  int pinValue = param.asInt();
  if (pinValue == 1) pressButton(closeCurtainsPin, pressDuration);
}

void redWallLedSensorThread(void* param) {
  bool isLastColorRestored = true;
  while (true) {
    if (digitalRead(ledRedWallPin) == HIGH) {
      led::addColorAsync(led::red, 0).get();
      while (digitalRead(ledRedWallPin)) {
        delay(10);
      }
      isLastColorRestored = false;
    }
    if (digitalRead(ledRedWallPin) == LOW && isLastColorRestored == false) {
      led::removeColorAsync(led::red).get();
      isLastColorRestored = true;
    }
    delay(10);
  }
}

void greenWallLedSensorThread(void* param) {
  bool isLastColorRestored = true;
  while (true) {
    if (digitalRead(ledGreenWallPin) == HIGH) {
      led::addColorAsync(led::green, 0).get();
      while (digitalRead(ledGreenWallPin) == HIGH) {
        delay(10);
      }
      isLastColorRestored = false;
    }
    if (digitalRead(ledGreenWallPin) == LOW && isLastColorRestored == false) {
      led::removeColorAsync(led::green).get();
      isLastColorRestored = true;
    }
    delay(10);
  }
}

void blueLedThread(void* param) {
  bool isLastColorRestored = true;
  while (true) {
    if (Blynk.connected() && WiFi.isConnected()) {
      led::addColorAsync(led::blue, 0).get();
    } else {
      while (!Blynk.connected() || !WiFi.isConnected()) {
        led::addColorAsync(led::blue, 0).get();
        delay(500);
        led::removeColorAsync(led::blue).get();
      }
    }
    delay(10);
  }
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
    // Serial.printf("Wifi Connection Handler Thread current stack size: %d , current Time: %d\n", wifiHandlerThreadStackSize - uxTaskGetStackHighWaterMark(NULL), xTaskGetTickCount());
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
    // Serial.printf("Blynk Connection Handler Thread current stack size: %d , current Time: %d\n", blynkHandlerThreadStackSize - uxTaskGetStackHighWaterMark(NULL), xTaskGetTickCount());
  }
}
