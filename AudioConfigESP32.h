#ifndef AUDIOCONFIGESP32_H
#define AUDIOCONFIGESP32_H
#include "mozzi_config_defines.h"

#if not IS_ESP32()
#error This header should be included for ESP32 architecture, only
#endif

#if (AUDIO_MODE == HIFI)
#error HIFI mode is not available for this CPU architecture (but check ESP32_AUDIO_OUT_MODE, and PDM_RESOLUTION)
#endif

// Default ouput implementation class
#define PLATFORM_OUTPUT_CLASS PlatformESP32_I2S

// Audio output options
#define INTERNAL_DAC 1 // output using internal DAC via I2S, output on pin 26
#define PT8211_DAC 2 // output using an external PT8211 DAC via I2S
#define PDM_VIA_I2S 3 // output PDM coded sample on the I2S data pin (pin 33, by default, configurable, below)

// Set output mode
#define ESP32_AUDIO_OUT_MODE PT8211_DAC

// For external I2S output, only: I2S_PINS
#define ESP32_I2S_BCK_PIN 27
#define ESP32_I2S_WS_PIN 25
#define ESP32_I2S_DATA_PIN 26
#define AUDIO_RATE_PLATFORM_DEFAULT 32000

#include <driver/i2s.h>
const i2s_port_t i2s_num = I2S_NUM_0;
/// User config end. Do not modify below this line

#if (ESP32_AUDIO_OUT_MODE == INTERNAL_DAC)
#define AUDIO_BITS 8
#define PDM_RESOLUTION 1
#elif (ESP32_AUDIO_OUT_MODE == PT8211_DAC)
#define AUDIO_BITS 16
#define PDM_RESOLUTION 1
#elif (ESP32_AUDIO_OUT_MODE == PDM_VIA_I2S)
#define AUDIO_BITS 16
#define PDM_RESOLUTION 4
#else
#error Invalid output mode configured in AudioConfigESP32.h
#endif

#define AUDIO_BIAS ((uint16_t) 1<<(AUDIO_BITS-1))
#define BYPASS_MOZZI_OUTPUT_BUFFER true

// We do not want to use int because it is 4 bytes -> we switch to 2 bytes instead!
//#define AudioOutputStorage_t int16_t


#endif        //  #ifndef AUDIOCONFIGESP_H
