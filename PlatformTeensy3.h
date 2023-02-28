/*
 * PlatformTeensy3.h
 *
 * Copyright 2012 Tim Barrass.
 *
 * This file is part of Mozzi.
 *
 * Mozzi by Tim Barrass is licensed under a Creative Commons
 * Attribution-NonCommercial-ShareAlike 4.0 International License.
 *
 */
#pragma once

#include "hardware_and_config.h"
#if IS_TEENSY3() && USE_LEGACY_GUTS == false
#include "PlatformCommon.h"

extern PLATFORM_OUTPUT_CLASS Mozzi;

/**
* @brief  Hardware specific implementation class for Teensy 3 processors providing PWM output
 */
class PlatformTeensy3 : public PlatformOutput {
  public:
    void start(int control_rate_hz);
    void stop();
    static unsigned long audioTicks();
    static unsigned long mozziMicros();
    static void audioHook();

    // external output API
    void setAudioRate(uint16_t rate);
    bool write(const AudioOutput out);
    bool canWrite();

#if (USE_AUDIO_INPUT == true)
    static int getAudioInput();    
#endif

};

#if (EXTERNAL_AUDIO_OUTPUT == true)
extern void audioOutput(const AudioOutput f);
#endif


#endif
