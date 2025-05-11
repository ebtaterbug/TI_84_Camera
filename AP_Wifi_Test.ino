#include <Arduino.h>
#include "TIManager.h"
#include "CameraModule.h"

// #define WAKE_PIN GPIO_NUM_3

TIManager tiManager;

void setup() {
  Serial.begin(115200);
  delay(3000);
    if (!setupCamera()) {        // initialises esp_camera with the pin map in CameraModule.cpp
    Serial.println("Camera init failed");
    while (true) {}            // stop here if it didn’t start
  }
  // Use internal pull-up; connect GPIO3 to GND to read LOW.
  // pinMode(WAKE_PIN, INPUT);
  // Serial.println("Setup complete.");
  tiManager.setup();
}

void loop() {
  tiManager.loop();

  // When GPIO9 is shorted to GND, the pin reads LOW.
  // if (digitalRead(WAKE_PIN) == LOW) {
  //   Serial.println("Detected 0V on WAKE_PIN, going to deep sleep...");
  //   delay(1000);

  //   // Enable wakeup on a HIGH transition of GPIO3
  //   esp_deep_sleep_enable_gpio_wakeup(1ULL << 3, ESP_GPIO_WAKEUP_GPIO_HIGH);
  //   esp_deep_sleep_start();
  // }
}