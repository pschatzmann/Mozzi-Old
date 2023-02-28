/*
 * PlatformRP2040.h
 * 
 * Mozzi Support for the Rasperry Pico for the Arduino Kernels which provide the full Rasperry Pico API
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

#if IS_RP2040() 

#include "PlatformCommon.h"
#include "AudioConfigRP2040.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "hardware/pwm.h"
#include "pico/time.h"

// Forward declarations of callbacks
extern PLATFORM_OUTPUT_CLASS Mozzi;
static PlatformRP2040 *mozziRP2040 = nullptr;
static bool defaultAudioOutputCallbackX(repeating_timer* ptr);
static bool defaultAudioInputCallbackX(repeating_timer* ptr);
static uint8_t max_channel_pico; // last valid channel with a pin defined

/**
* @brief  Hardware specific implementation class for the Raspberry Pico API - Please note that the standard Arduino implementation is based on MBed and does not
* use this implementation.
*
* Design decisions: 
* - avoid preprocessor #if whenever possible. On this architecture we have enough memory so we prefer readability over efficiency and the overhead of an executed if is minimal
* - if we cannot avoid preprocerror #if we prefer to handle this full method implementatioins.
*/

class PlatformRP2040 : public PlatformOutput {
  friend bool defaultAudioOutputCallbackX(repeating_timer* ptr);

  public:

    void start(int control_rate_hz) {
      mozziRP2040 = this;
      setupADC(); 
      Common.startControl(control_rate_hz);
      startAudio();
    }

    void stop() {
        if (timer!=nullptr){
          cancel_repeating_timer(&timer);
        }        
        if (USE_AUDIO_INPUT == true){
          if (timer!=nullptr){
            cancel_repeating_timer(&timer);
          }        
        }
        stopPWM(pins());
        mozziRP2040 = nullptr;
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

    bool write(const AudioOutput out){
      return Common.write(out);
    }

     bool canWrite(){
      return Common.canBufferAudioOutput();
    }
   
#if USE_AUDIO_INPUT == true

    // Provides the analog value w/o delay
    static int getAudioInput() { 
      return PlatformCommon::getAudioInput();
    }

#endif

  protected:
    uint pwm_slice_num;
    alarm_pool_t *ap;
    repeating_timer_t timer=nullptr;
    repeating_timer_t timer_input=nullptr;
    char msg[80];
    uint16_t audio_rate = AUDIO_RATE;
    PlatformCommon Common;
    int16_t channel_pins[AUDIO_CHANNELS];

    //-----------------------------------------------------------------------------------------------------------------
    /// Setup
    //-----------------------------------------------------------------------------------------------------------------

    void startAudio() {
      // this supports all AUDIO_MODE settings
      setupPWM(pins());
      // setup timer for defaultAudioOutput
      setupTimer();
    }


    void setupPWMPin(int pin, pwm_config &cfg){
      uint slice = pwm_gpio_to_slice_num(pin);
      uint channel = pwm_gpio_to_channel(pin);

      gpio_set_function(pin, GPIO_FUNC_PWM);
      pwm_init(slice, &cfg, true);

      // set initial output value 
      pwm_set_chan_level(slice, channel, 0); 

    }

    void setupPWM(const int16_t pins[]) {
      // setup pwm frequency
      pwm_config pico_pwm_config = pwm_get_default_config();
      int wrap_value = AUDIO_BIAS*2; // amplitude of square wave (pwm values -amplitude to amplitude) for one byte
      float pwmClockDivider = static_cast<float>(clock_get_hz(clk_sys)) / (PWM_RATE * wrap_value);
      pwm_config_set_clkdiv(&pico_pwm_config, pwmClockDivider);
      pwm_config_set_clkdiv_mode(&pico_pwm_config, PWM_DIV_FREE_RUNNING);
      pwm_config_set_wrap (&pico_pwm_config, wrap_value);

      // setup pins
      for (int j=0;j<AUDIO_CHANNELS;j++){
        int16_t gpio = pins[j];
        if  (gpio >- 1)  {
            sprintf(msg,"Channel %d -> pin %d", j, gpio);
            Serial.println(msg);
            setupPWMPin(gpio, pico_pwm_config);
            max_channel_pico = j + 1;
        } else {
            sprintf(msg,"Channel %d -> pin %d is already defined", j, gpio);
            Serial.println(msg);
        }
      }
    }

    void stopPWM(const int16_t pins[]){
        // stop all pins
        for (int j=0;j<max_channel_pico;j++){
          int gpio = pins[j];
          if  (gpio >- 1)  {
            uint slice = pwm_gpio_to_slice_num(gpio);
            pwm_set_enabled(slice, false);
          } 
        }
    }

    void setupTimer() {
        uint64_t time = 1000000UL / audio_rate;
        if (!add_repeating_timer_us(-time, defaultAudioOutputCallbackX, nullptr, &timer)){
          Serial.println("Error: alarm_pool_add_repeating_timer_us failed; no alarm slots available");
        }

        if (USE_AUDIO_INPUT == true) {
          if (!add_repeating_timer_us(-time, defaultAudioInputCallbackX, nullptr, &timer_input)){
            Serial.println("Error: alarm_pool_add_repeating_timer_us failed; no alarm slots available");
          }
        }
    }


    //-----------------------------------------------------------------------------------------------------------------
    // Input
    //-----------------------------------------------------------------------------------------------------------------

    void  setupADC() {
      if (USE_AUDIO_INPUT){
        adc_init();
        // Make sure GPIO is high-impedance, no pullups etc
        adc_gpio_init(AUDIO_INPUT_PIN);
        // Select ADC input 0 (GPIO26)
        adc_select_input(AUDIO_CHANNEL_IN);
      }
    }

    //-----------------------------------------------------------------------------------------------------------------
    // Output
    //-----------------------------------------------------------------------------------------------------------------

    void writePWM(int16_t pin, int16_t value){
      uint slice = pwm_gpio_to_slice_num(pin);
      uint channel = pwm_gpio_to_channel(pin);
      pwm_set_chan_level(slice, channel, value);
    }

    void audioOutput(const AudioOutput value) {
        // output values to pins
        const int16_t *pins_ptr = pins();
        for (int j=0;j<max_channel_pico;j++){
            AudioOutputStorage_t v = value[j];
            writePWM(pins_ptr[j], v + AUDIO_BIAS);    
        }
    }

};


//-----------------------------------------------------------------------------------------------------------------
/// Callbacks 
//-----------------------------------------------------------------------------------------------------------------

bool defaultAudioInputCallbackX(repeating_timer* ptr) {
    // pico adc_read() provides max value 65536
    int value = map(adc_read(),0,65536,-AUDIO_BIAS,AUDIO_BIAS)
    PlatformCommon::setAudioInput(value());
}

#if EXTERNAL_AUDIO_OUTPUT == true

bool defaultAudioOutputCallbackX(repeating_timer* ptr) {
  if(mozziRP2040!=nullptr){
    audioOutput(output_buffer.read());
  }
  return true;
}

#else

bool defaultAudioOutputCallbackX(repeating_timer* ptr) {
  if(mozziRP2040!=nullptr){
    mozziRP2040->audioOutput(output_buffer.read());
  }
  return true;
}

#endif

#endif  // IS_RP2040
