#ifndef CAMERA_MODULE_H
#define CAMERA_MODULE_H

#include <esp_camera.h>

extern camera_config_t config;



bool setupCamera();
bool captureImage(uint8_t** buffer, size_t* length);

#endif
