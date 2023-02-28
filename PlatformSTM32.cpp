/*
 * MozziStm32.cpp
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
#if IS_STM32() && USE_LEGACY_GUTS == false

#include "PlatformSTM32.h"
#include "HardwareTimer.h"
#include "AudioConfigSTM32.h"
//#include <STM32ADC.h>  // Disabled, here. See AudioConfigSTM32.h


STM32ADC adc(ADC1);
uint8_t stm32_current_adc_pin;
PlatformCommon Common;
static uint16_t audio_rate = AUDIO_RATE;

void PlatformSTM32::setAudioRate(uint16_t rate){
    audio_rate = rate;
}

bool PlatformSTM32::write(const AudioOutput out){
  return Common.write(out);
}

bool PlatformSTM32::canWrite(){
  return Common.canBufferAudioOutput();
}


#if (EXTERNAL_AUDIO_OUTPUT != true)
inline void audioOutput(const AudioOutput f)
{
#if (AUDIO_MODE == HIFI)
  pwmWrite(AUDIO_CHANNEL_1_PIN, (f.l()+AUDIO_BIAS) & ((1 << AUDIO_BITS_PER_CHANNEL) - 1));
  pwmWrite(AUDIO_CHANNEL_1_PIN_HIGH, (f.l()+AUDIO_BIAS) >> AUDIO_BITS_PER_CHANNEL);
#else
  pwmWrite(AUDIO_CHANNEL_1_PIN, f.l()+AUDIO_BIAS);
#if (AUDIO_CHANNELS > 1)
  pwmWrite(AUDIO_CHANNEL_2_PIN, f.r()+AUDIO_BIAS);
#endif
#endif
}
#endif

//-----------------------------------------------------------------------------------------------------------------
////// BEGIN AUDIO INPUT code ////////
//-----------------------------------------------------------------------------------------------------------------
#if (USE_AUDIO_INPUT == true)

uint8_t adc_count = 0;

int PlatformSTM32::getAudioInput() { return PlatformCommon::getAudioInput(); }

static void startFirstAudioADC() {
  uint8_t dummy = AUDIO_INPUT_PIN;
  adc.setPins(&dummy, 1);
  adc.startConversion();
}


static void startSecondAudioADC() {
  uint8_t dummy = AUDIO_INPUT_PIN;
  adc.setPins(&dummy, 1);
  adc.startConversion();
}

static void receiveSecondAudioADC() {
    setAudioInput(adc.getData())
}

void stm32_adc_eoc_handler()
{
  switch (adc_count) {
  case 0:
    // 6us
    receiveSecondAudioADC();
    adcReadSelectedChannels();
    break;

  case 1:
    // <2us, <1us w/o receive
    // receiveFirstControlADC();
    startSecondControlADC();
    break;

  case 2:
    // 3us
    receiveSecondControlADC();
    startFirstAudioADC();
    break;
  }
  adc_count++;
}

#endif

//-----------------------------------------------------------------------------------------------------------------
////// Ouptu & Control
//-----------------------------------------------------------------------------------------------------------------

bool canBufferAudioOutput(){
  return Common.canBufferAudioOutput();
} 

void bufferAudioOutput(const AudioOutput_t f){
  Common.bufferAudioOutput(f);
}

#if (BYPASS_MOZZI_OUTPUT_BUFFER != true)
static void CACHED_FUNCTION_ATTR defaultAudioOutput() {
#if (USE_AUDIO_INPUT == true)
  adc_count = 0;
  startSecondAudioADC();
#endif
  audioOutput(output_buffer.read());
}
#endif

#if (EXTERNAL_AUDIO_OUTPUT == true)
HardwareTimer audio_update_timer(2);
#else
HardwareTimer audio_update_timer(AUDIO_UPDATE_TIMER);
HardwareTimer audio_pwm_timer(AUDIO_PWM_TIMER);
#endif


static void startAudioStandard() {

  audio_update_timer.pause();
  //audio_update_timer.setPeriod(1000000UL / AUDIO_RATE);
  // Manually calculate prescaler and overflow instead of using setPeriod, to avoid rounding errors
  uint32_t period_cyc = F_CPU / audio_rate;
  uint16_t prescaler = (uint16_t)(period_cyc / 65535 + 1);
  uint16_t overflow = (uint16_t)((period_cyc + (prescaler / 2)) / prescaler);
  audio_update_timer.setPrescaleFactor(prescaler);
  audio_update_timer.setOverflow(overflow);
  audio_update_timer.setChannel1Mode(TIMER_OUTPUT_COMPARE);
  audio_update_timer.setCompare(TIMER_CH1,
                                1); // Interrupt 1 count after each update
  audio_update_timer.attachCompare1Interrupt(defaultAudioOutput);
  audio_update_timer.refresh();
  audio_update_timer.resume();

#if (EXTERNAL_AUDIO_OUTPUT != true)
    pinMode(AUDIO_CHANNEL_1_PIN, PWM);
  #if (AUDIO_MODE == HIFI)
    pinMode(AUDIO_CHANNEL_1_PIN_HIGH, PWM);
  #elif (AUDIO_CHANNELS > 1)
    pinMode(AUDIO_CHANNEL_2_PIN, PWM);
  #endif

  #define MAX_CARRIER_FREQ (F_CPU / (1 << AUDIO_BITS_PER_CHANNEL))
  #if MAX_CARRIER_FREQ < AUDIO_RATE
    #error Configured audio resolution is definitely too high at the configured audio rate (and the given CPU speed)
  #elif MAX_CARRIER_FREQ < (AUDIO_RATE * 3)
    #warning Configured audio resolution may be higher than optimal at the configured audio rate (and the given CPU speed)
  #endif

  if (MAX_CARRIER_FREQ < (audio_rate * 5)) {
    // Generate as fast a carrier as possible
    audio_pwm_timer.setPrescaleFactor(1);
  } else {
    // No point in generating arbitrarily high carrier frequencies. In fact, if
    // there _is_ any headroom, give the PWM pin more time to swing from HIGH to
    // LOW and BACK, cleanly
    audio_pwm_timer.setPrescaleFactor((int)MAX_CARRIER_FREQ / (audio_rate * 5));
  }
  audio_pwm_timer.setOverflow(1 << AUDIO_BITS_PER_CHANNEL); // Allocate enough room to write all
                                                              // intended bits
#endif

}

static void startControl(unsigned int control_rate_hz) {
  update_control_timeout = audio_rate / control_rate_hz;
}

//-----------------------------------------------------------------------------------------------------------------
// Class Methods
//-----------------------------------------------------------------------------------------------------------------

void PlatformSTM32::start(int control_rate_hz) {
  setupMozziADC(); // you can use setupFastAnalogRead() with FASTER or FASTEST
                   // in setup() if desired (not for Teensy 3.* )
  // delay(200); // so AutoRange doesn't read 0 to start with
  startControl(control_rate_hz);
#if (AUDIO_MODE == STANDARD) || (AUDIO_MODE == STANDARD_PLUS) ||               \
    IS_STM32 // Sorry, this is really hacky. But on STM32 regular and HIFI
               // audio modes are so similar to set up, that we do it all in one
               // function.
  startAudioStandard();
#elif (AUDIO_MODE == HIFI)
  startAudioHiFi();
#endif
}

void PlatformSTM32::stop() {
  audio_update_timer.pause();
  // ps - nointerrupts was never called so the following is not necessary: 
  // interrupts();
}

unsigned long PlatformSTM32::audioTicks() {
  return PlatformCommon::audioTicks();
}

unsigned long PlatformSTM32::mozziMicros() { 
  return PlatformCommon::mozziMicros();
}

void PlatformSTM32::audioHook() // 2us on AVR excluding updateAudio()
{
  PlatformCommon::audioHook();
}

#endif
