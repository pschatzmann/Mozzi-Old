/*
 * PlatformMBED.cpp
 * 
 * Mozzi Support for Microporcessors which use the ARM Mbed OS (e.g. Raspberry Pico)
 *
 * Design Decisions:
 * - We avoid preprocessor statements because ARM processors have enough resources and because if is easier to catch compile errors
 * - We avoid any low level APIs - to be as understandable as possible
 * - These decisions are valid for this implementation only!
 *
 * Copyright 2021 Phil Schatzmann.
 * 
 * This file is part of Mozzi.
 *
 * Mozzi by Tim Barrass is licensed under a Creative Commons
 * Attribution-NonCommercial-ShareAlike 4.0 International License.
 *()
 */

#pragma once
#include "hardware_and_config.h"

#if IS_MBED() 

#include "PlatformCommon.h"
#include "pinDefinitions.h"
#include "mbed.h"

extern PLATFORM_OUTPUT_CLASS Mozzi;
// Forward declarations of callbacks
static void defaultAudioInputCallbackX();
static void defaultAudioOutputCallbackX();

/**
* @brief  Hardware specific implementation class for Arm Mbed based processors using PWM output
*/
class PlatformMBED  : public PlatformOutput {
  friend void defaultAudioInputCallbackX();
  friend void defaultAudioOutputCallbackX();

  public:
      void start(int control_rate_hz) {
        Common.startControl(control_rate_hz);
        startAudio(channel_pins);

        sprintf(msg,"Mozzi started on %d channels", max_channel_mbed);
        Serial.println(msg);
      }

      void stop() {
        ticker.detach(); // it does not hurt to call this even if it has not been started

        // stop all pins
        for (int j=0;j<max_channel_mbed;j++){
          if (pwm_pins[j] != nullptr)  {
            pwm_pins[j]->suspend();
          } 
        }
        stopInput();
      }

      static unsigned long audioTicks() {
        return PlatformCommon::audioTicks();
      }

      static unsigned long mozziMicros() { 
        return PlatformCommon::mozziMicros();;
      }

      static void audioHook()  {
        PlatformCommon::audioHook();
      }

      const int channels() const {
          return Common.channels();
      }

      const int16_t* pins() {
        return channel_pins;
      }

      void setPin(uint8_t idx, int16_t pin){
        channel_pins[idx] = pin;
      }

      void setAudioRate(uint16_t rate){
        audio_rate = rate;
      }

      bool canWrite(){
        return Common.canBufferAudioOutput();
      }

      bool write(const AudioOutput out){
        return Common.write(out);
      }

#if USE_AUDIO_INPUT == true

      // Provides the analog value w/o delay
      static int getAudioInput() { 
        return PlatformCommon::getAudioInput();
      }

#endif

  protected:
    //-----------------------------------------------------------------------------------------------------------------
    /// Variables
    //-----------------------------------------------------------------------------------------------------------------
    PlatformCommon Common;
    uint16_t audio_rate = AUDIO_RATE;
    int16_t channel_pins[AUDIO_CHANNELS];

    mbed::Ticker ticker; // calls a callback repeatedly with a timeout
    mbed::Ticker inputTicker; // calls a callback repeatedly with a timeout
    mbed::PwmOut *pwm_pins[AUDIO_CHANNELS];
    const uint MAX_VALUE = 2*AUDIO_BIAS;
    uint8_t max_channel_mbed = 0; // last valid used channel with a pin defined
    char msg[80];

    //-----------------------------------------------------------------------------------------------------------------
    // Setup
    //-----------------------------------------------------------------------------------------------------------------

    void setupTimer() {
        // to test we use an interval of 1 sec
        //ticker.attach(defaultAudioOutputCallback,1.0);

        // We use 1000000l / AUDIO_RATE => 1000000l / 16000 => 62 microsends 
        long wait_time = 1000000l / audio_rate;
        ticker.attach_us(defaultAudioOutputCallbackX, wait_time);
    }


    void setupPWMPin(mbed::PwmOut &pin){
      unsigned long period = 1000000l / PWM_RATE;  // -> 30.517578125 microseconds
      pin.period_us(period);  
      pin.write(0.0f);  // 0% duty cycle ->  
      pin.resume(); // in case it was suspended before
    }


    void setupPWM(const int16_t pins[]) {
      for (int j=0;j<AUDIO_CHANNELS;j++){
        int16_t gpio = pins[j];
        if  (pwm_pins[j] == nullptr && gpio >- 1)  {
          mbed::PwmOut *pin = new mbed::PwmOut(digitalPinToPinName(gpio));
          pwm_pins[j] = pin;
          if (pin!=nullptr) {
            sprintf(msg,"Channel %d -> pin %d", j, gpio);
            Serial.println(msg);
            setupPWMPin(*pin);
            max_channel_mbed = j + 1;
          } else {
            sprintf(msg,"Channel %d -> pin could not be defined", j);
            Serial.println(msg);
          }
        } else {
            sprintf(msg,"Channel %d -> pin %d is already defined", j, gpio);
            Serial.println(msg);
        }
      }
    }

    //-----------------------------------------------------------------------------------------------------------------
    // Start - Stop - Control


    void startAudio(const int16_t pins[]) {
      if (USE_AUDIO_INPUT) {
        setupInput();
      }

      // this supports all AUDIO_MODE settings
      if (!EXTERNAL_AUDIO_OUTPUT)
        setupPWM(pins);

      // setup timer for defaultAudioOutput
      setupTimer();
    }

    inline void advanceControlLoop() {
      if (Common.advanceControlLoop()){
          //adcStartReadCycle();
      }
    }

    //-----------------------------------------------------------------------------------------------------------------
    /// Input -> This might not be optimal but we just use the MBED AnaSerialIn )
    //-----------------------------------------------------------------------------------------------------------------

    #if USE_AUDIO_INPUT == true

    mbed::AnalogIn *analog_in = nullptr;

    void defaultAudioInputCallback() {
      int value = analog_in->read_u16(); 
      // range 0x0, 0xFFFF -> -AUDIO_BIAS, +AUDIO_BIAS
      PlatformCommon::setAudioInput(map(value,0x0,0xFFFF,-AUDIO_BIAS,AUDIO_BIAS))
    }

    void setupInput() {
        // We use 1000000l / AUDIO_RATE => 1000000l / 16000 => 62 microsends 
        long wait_time = 1000000l / audio_rate;
        ticker.attach_us(defaultAudioInputCallbackX, wait_time);

        if (analog_in==nullptr){
          analog_in = new mbed::AnalogIn(digitalPinToPinName(AUDIO_INPUT_PIN));
        }
    }

    void stopInput() {
      inputTicker.detach();  // it does not hurt to call this even if it has not been started
      // rmove analog pin
      if (analog_in!=nullptr)
        delete analog_in;
    }

    #elif USE_AUDIO_INPUT == false

    void setupInput() {}
    void stopInput() {}

    #endif

    //-----------------------------------------------------------------------------------------------------------------
    /// Output 
    //-----------------------------------------------------------------------------------------------------------------

    // output an n channels
    void audioOutput(const AudioOutput value) {
        #if (AUDIO_MODE == HIFI)
          // audioOutputHIFI(value[0]);
          #error "HIFI not supported"
        #else
          // output values to pins
          for (int j=0;j<max_channel_mbed;j++){
            AudioOutputStorage_t v = value[j] + AUDIO_BIAS;
            float float_value = static_cast<float>(v) / MAX_VALUE;
            pwm_pins[j]->write(float_value);    // pwm the value is between 0.0 and 1.0 
          }
        #endif
    }


};

//-----------------------------------------------------------------------------------------------------------------
/// Callbacks 
//-----------------------------------------------------------------------------------------------------------------

#if EXTERNAL_AUDIO_OUTPUT == true

void defaultAudioOutputCallbackX(){
    if (!output_buffer.isEmpty()){
      audioOutput(output_buffer.read());
    }
}
#else

void defaultAudioOutputCallbackX(){
    if (!output_buffer.isEmpty()){
      Mozzi.audioOutput(output_buffer.read());
    }
}

#endif


#if USE_AUDIO_INPUT == true
void defaultAudioInputCallbackX(){
  Mozzi.defaultAudioInputCallback();
}
#endif


#endif  // IS_MBED
