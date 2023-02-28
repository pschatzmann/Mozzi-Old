#ifndef AUDIOCONFIGRP2040_H
#define AUDIOCONFIGRP2040_H
#include "mozzi_config_defines.h"


// Sample frequency
#define AUDIO_RATE_PLATFORM_DEFAULT 32768

// this is driving the  
#define AUDIO_BITS 8
// Used internally to put the 0-biased generated audio into the centre of the output range (12 bits) -> Range from - 2048 to + 2048
#define AUDIO_BIAS ((uint16_t) 128)

// the PWM counter runs at clk_sys / div; The maximum clk_sys clock speed is 133MHz at normal core voltage
// PWM carrier frequency should be well out of hearing range, about 5 times the nyquist frequency if possible. 
#define PWM_RATE 60000

// Pins - Please make sure they are on the same channel!
#define AUDIO_CHANNEL_1_PIN 2
#define AUDIO_CHANNEL_2_PIN 3
#define AUDIO_CHANNEL_1_PIN_HIGH 3

// Overwrite Input Pins!
#if USE_AUDIO_INPUT == true
#ifdef AUDIO_INPUT_PIN
#undef AUDIO_INPUT_PIN 
//#warning We use the AUDIO_INPUT_PIN defined in AudioConfigRP2040.h
#endif
// ADC input. 0...3 are GPIOs 26...29 respectively. Input 4 is the onboard temperature sensor.
#define AUDIO_INPUT_PIN 26
#define AUDIO_CHANNEL_IN 0
#endif


// We do not want to use int because it is 4 bytes -> we switch to 2 bytes instead!
#define AudioOutputStorage_t int16_t

// Select ouput implementation class: PlatformRP2040_I2S, PlatformRP2040
#define PLATFORM_OUTPUT_CLASS PlatformRP2040

// The platform supports setPin() calls to assign specific pins to channels - DO NOT CHANGE
#define SUPPORTS_SET_PIN true

// Compantibility checks
#if EXTERNAL_AUDIO_OUTPUT == true && PLATFORM_OUTPUT_CLASS == PlatformRP2040_I2S
#error "EXTERNAL_AUDIO_OUTPUT not supported for PlatformRP2040_I2S - lease use #define PLATFORM_OUTPUT_CLASS PlatformRP2040 instead"
#endif

#if USE_AUDIO_INPUT == true && PLATFORM_OUTPUT_CLASS == PlatformRP2040_I2S
#error "USE_AUDIO_INPUT not supported for PlatformRP2040_I2S - lease use #define PLATFORM_OUTPUT_CLASS PlatformRP2040 instead"
#endif

#if (AUDIO_MODE == HIFI)
#error "HIFI not supported"
#endif

#endif  // AUDIOCONFIGRP2040_H