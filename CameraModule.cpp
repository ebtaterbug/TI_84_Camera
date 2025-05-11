#include <Arduino.h>
#include "CameraModule.h"

// Camera pins (Xiao ESP32C3 Sense)
#define PWDN_GPIO   -1
#define RESET_GPIO  -1
#define XCLK_GPIO   10
#define SIOD_GPIO   40
#define SIOC_GPIO   39
#define Y9_GPIO     48
#define Y8_GPIO     11
#define Y7_GPIO     12
#define Y6_GPIO     14
#define Y5_GPIO     16
#define Y4_GPIO     18
#define Y3_GPIO     17
#define Y2_GPIO     15
#define VSYNC_GPIO  38
#define HREF_GPIO   47
#define PCLK_GPIO   13

camera_config_t config;

bool setupCamera() {
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer   = LEDC_TIMER_0;
    config.pin_d0 = Y2_GPIO;
    config.pin_d1 = Y3_GPIO;
    config.pin_d2 = Y4_GPIO;
    config.pin_d3 = Y5_GPIO;
    config.pin_d4 = Y6_GPIO;
    config.pin_d5 = Y7_GPIO;
    config.pin_d6 = Y8_GPIO;
    config.pin_d7 = Y9_GPIO;
    config.pin_xclk  = XCLK_GPIO;
    config.pin_pclk  = PCLK_GPIO;
    config.pin_vsync = VSYNC_GPIO;
    config.pin_href  = HREF_GPIO;
    config.pin_sccb_sda = SIOD_GPIO;
    config.pin_sccb_scl = SIOC_GPIO;
    config.pin_pwdn  = PWDN_GPIO;
    config.pin_reset = RESET_GPIO;
    config.xclk_freq_hz = 20000000;
    config.pixel_format = PIXFORMAT_JPEG;
    config.fb_location  = CAMERA_FB_IN_PSRAM;
    config.grab_mode    = CAMERA_GRAB_LATEST;
    config.frame_size   = FRAMESIZE_SVGA;
    config.jpeg_quality = 10;
    config.fb_count     = 2;

    if (esp_camera_init(&config) != ESP_OK) {
        Serial.println("Camera init failed");
        return false;
    }
    Serial.println("Camera ready");
    return true;
}

bool captureImage(uint8_t** buffer, size_t* length) {
    camera_fb_t* fb = esp_camera_fb_get();
    if (!fb) {
        Serial.println("Capture failed");
        return false;
    }
    *buffer = (uint8_t*)malloc(fb->len);
    if (!*buffer) {
        Serial.println("Alloc failed");
        esp_camera_fb_return(fb);
        return false;
    }
    memcpy(*buffer, fb->buf, fb->len);
    *length = fb->len;
    esp_camera_fb_return(fb);
    return true;
}
