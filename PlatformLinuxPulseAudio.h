/*
 * PlatformRP2040.h
 * 
 * I2S Mozzi Support for the Rasperry Pico for the Arduino Kernels which provide the full Rasperry Pico API
 * ATTENTION: The standard Arduino implementation uses the ARM Mbed! So this would be the wrong place
  
 * Copyright 2021 Phil Schatzmann.
 * 
 * This file is part of Mozzi.
 *
 * Mozzi by Tim Barrass is licensed under a Creative Commons
 * Attribution-NonCommercial-ShareAlike 4.0 International License.
 *
 */

#pragma once
#include "hardware_and_config.h"

#if IS_LINUX() 

#include "PlatformCommon.h"
#include "AudioConfigLINUX.h"
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

#include <pulse/simple.h>
#include <pulse/error.h>
#include <pulse/gccmacro.h>

static bool linux_active;
static pa_simple *pulse_audio_connection = nullptr;
static int linux_error;
static uint64_t tick_count = 0;

// We can not use the regular updateAudio() and updateAudio() because they do not exist at compile time
typedef AudioOutput_t (*updateAudioLinux)();
typedef void (*updateControlLinux)();
static updateAudioLinux updateAudioLinuxCB = nullptr;
static updateControlLinux updateControlLinuxCB = nullptr;

/**
* @brief Hardware specific implementation class for Linux using the PulseAudio simple API. The signature of most void methods has
* been altered to return a boolean, so that we can easily check if the call has failed. This is quite useful if the code is 
* executed interatively e.g. in Jupyterlab
*/
class PlatformLinuxPulseAudio : public PlatformOutput {

  public:
    
    ~PlatformLinuxPulseAudio(){
      stop();
    }

    /// External API:  Defines the updateAudio() and updateAudio() methods to be used - This must be called before the start method!
    boolean start(updateAudioLinux ua, updateControlLinux uc, int control_rate_hz=CONTROL_RATE){
      updateAudioLinuxCB = ua;
      updateControlLinuxCB = uc;
      return start(control_rate_hz);
    }

    boolean start(int control_rate_hz) {
      this->control_rate = control_rate_hz;
      return start();
    }

    boolean start() {
      update_control_timeout = ss.rate / this->control_rate;
      // if the code is not built as library we expect updateControl and updateAudio to be defined!
      #ifdef NO_LIB
        if (updateAudioLinuxCB==nullptr){
          updateAudioLinuxCB = updateAudio;
        }
        if (updateControlLinuxCB==nullptr){
          updateControlLinuxCB = updateControl;
        }
      #endif
      /* Create a new playback stream */
      tick_count = 0;
      if (updateAudioLinuxCB==nullptr || updateControlLinuxCB==nullptr){
        linux_active = false;
        return false;
      }
      bool ok = true;
      pulse_audio_connection = pa_simple_new(NULL, NULL, PA_STREAM_PLAYBACK, NULL, "playback", &ss, NULL, NULL, &linux_error);
      if (pulse_audio_connection==nullptr) {
        ok = false;
      }
      linux_active = ok;
      return ok;
    }

    boolean stop() {
      linux_active = false;
      if (pulse_audio_connection) {
        pa_simple_drain(pulse_audio_connection, &linux_error);
        pa_simple_free(pulse_audio_connection);
      }
      return true;
    }

    /// external API: e.g from Jupyter
    boolean play(){
      while(linux_active){
        audioHook();
      }
      return linux_active;
    } 

    /// external API: e.g from Jupyter
    boolean play(int seconds){
      uint64_t end = millis() + (seconds*1000);
      while(millis()<end && linux_active){
        audioHook();
      }
      return linux_active;
    } 

    static unsigned long audioTicks() {
      return tick_count; 
    }

    static unsigned long mozziMicros() { 
        return tick_count * MICROS_PER_AUDIO_TICK;
    }

    /// we fill the output buffer and write it out pulse audio
    static void audioHook()  {
      if (linux_active){
        const int BUFFER_SIZE = 512;
        AudioOutput buffer[BUFFER_SIZE];
        for (int j=0;j<BUFFER_SIZE;j++){
          if (!update_control_counter) {
              update_control_counter = update_control_timeout;
              updateControlLinuxCB();
          } else {
              --update_control_counter;
          }
          buffer[j] = updateAudioLinuxCB();
          tick_count++;
        }
        audioOutput(buffer, BUFFER_SIZE);
      }
    }

    const int channels() const {
        return AUDIO_CHANNELS;
    }

    uint16_t audioRate() {
      return ss.rate;
    }

    void setAudioRate(uint16_t rate){
       ss.rate = rate;
    }

    unsigned int controlRate() {
      return control_rate;
    }

    bool write(const AudioOutput out){
      audioOutput(out);
      return true;
    }

    bool canWrite(){
       return linux_active;
    }

    int errorCode(){
      return linux_error;
    }
   
  protected:
    unsigned int control_rate;
    pa_sample_spec ss = {
        .format = PA_SAMPLE_S16LE,
        .rate = AUDIO_RATE,
        .channels = AUDIO_CHANNELS
    };
    int ret = 1;


    static void audioOutput(const AudioOutput value) {
      if (linux_active){
        tick_count++;
        pa_simple_write(pulse_audio_connection,(const void*) &value, (size_t) sizeof(AudioOutput), &linux_error); 
      }
    }

    static void audioOutput(void* buffer, size_t sample_count) {
      if (linux_active){
        pa_simple_write(pulse_audio_connection,buffer, (size_t) sizeof(AudioOutput)*sample_count, &linux_error); 
      }
    }
};

extern PLATFORM_OUTPUT_CLASS Mozzi;

#endif  // IS_RP2040
