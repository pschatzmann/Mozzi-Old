#pragma once
/*
 * MozziAvr.cpp
 *
 * Copyright 2012 Tim Barrass.
 *
 * This file is part of Mozzi.
 *
 * Mozzi by Tim Barrass is licensed under a Creative Commons
 * Attribution-NonCommercial-ShareAlike 4.0 International License.
 *
 */
#include "hardware_and_config.h"

#if IS_AVR() && USE_LEGACY_GUTS == false

#include "utility/FrequencyTimer2.h"
#include "utility/TimerOne.h"
#include "PlatformCommon.h"
#include "mozzi_analog.h"


#if (F_CPU != 16000000)
#warning                                                                       \
    "Mozzi has been tested with a cpu clock speed of 16MHz on Arduino and 48MHz on Teensy 3!  Results may vary with other speeds."
#endif

extern PLATFORM_OUTPUT_CLASS Mozzi;

// Forward declarations for locally used methods
void setupMozziADC(uint8_t speed);
void startAudioStandard();
void startAudioHiFi();
void stopTimers();


/**
 * @brief Hardware specific implementation for AVR Processors to generate PWM  
 */

class PlatformAVR : public PlatformOutput {
  public:
    void start(int control_rate_hz);
    void stop();
    static unsigned long audioTicks();
    static unsigned long mozziMicros();
    static void audioHook();

    bool canWrite();
    bool write(const AudioOutput out);
    void setAudioRate(uint16_t rate);
    
#if (USE_AUDIO_INPUT == true)
    static int getAudioInput();    
#endif

};


#endif
