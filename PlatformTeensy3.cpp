/*
 * PlatformTeensy3.cpp
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
#if IS_TEENSY3() && USE_LEGACY_GUTS == false

#include "PlatformTeensy3.h"
#include "IntervalTimer.h" // required from http://github.com/pedvide/ADC for Teensy 3.*
#include <ADC.h>
#include "AudioConfigTeensy3_12bit.h"


#if ( F_CPU != 48000000) 
#warning                                                                       \
    "Mozzi has been tested with a cpu clock speed of 16MHz on Arduino and 48MHz on Teensy 3!  Results may vary with other speeds."
#endif


ADC *adc; // adc object
uint8_t teensy_pin;
PlatformCommon Common;
static uint16_t audio_rate = AUDIO_RATE;

void PlatformTeensy3::setAudioRate(uint16_t rate){
    audio_rate = rate;
}

bool PlatformTeensy3::write(const AudioOutput out){
  return Common.write(out);
}

bool PlatformTeensy3::canWrite(){
  return Common.canBufferAudioOutput();
}


//-----------------------------------------------------------------------------------------------------------------
////// BEGIN AUDIO INPUT code ////////
//-----------------------------------------------------------------------------------------------------------------
#if (USE_AUDIO_INPUT == true)

// ring buffer for audio input
static boolean audio_input_is_available;
uint8_t adc_count = 0;

int PlatformTeensy3::getAudioInput() { 
  return PlatformCommon::getAudioInput();
 }

static void startFirstAudioADC() {
  adc->startSingleRead(AUDIO_INPUT_PIN); // ADC lib converts pin/channel in startSingleRead
}

static void startSecondAudioADC() {
  adc->startSingleRead(AUDIO_INPUT_PIN);
}

static void receiveSecondAudioADC() {
  Common.setAudioInput(adc->readSingle());
}

void adc0_isr(void)
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

    //      	case 3:
    // invisible
    //      	receiveFirstAudioADC();
    //      	break;
  }
  adc_count++;
}

#endif

//-----------------------------------------------------------------------------------------------------------------
// Control
//-----------------------------------------------------------------------------------------------------------------


bool canBufferAudioOutput(){
  return Common.canBufferAudioOutput();
} 

void bufferAudioOutput(const AudioOutput_t f){
  Common.bufferAudioOutput(f);
}

static void startControl(unsigned int control_rate_hz) {
  Common.startControl(control_rate_hz);
}

//-----------------------------------------------------------------------------------------------------------------
// Output
//-----------------------------------------------------------------------------------------------------------------
static void CACHED_FUNCTION_ATTR defaultAudioOutput() {
  #if (USE_AUDIO_INPUT == true)
    adc_count = 0;
    startSecondAudioADC();
  #endif
  audioOutput(output_buffer.read());
}

#if (EXTERNAL_AUDIO_OUTPUT != true)
inline void audioOutput(const AudioOutput f)
{
  analogWrite(AUDIO_CHANNEL_1_PIN, f.l()+AUDIO_BIAS);
}
#endif

#if (AUDIO_MODE == STANDARD) || (AUDIO_MODE == STANDARD_PLUS) 
IntervalTimer timer1;

static void startAudioStandard() {
  adc->adc0->setAveraging(ADC_AVERAGING);
  adc->adc0->setConversionSpeed(
      ADC_CONVERSION_SPEED::MED_SPEED); // could be HIGH_SPEED, noisier

  analogWriteResolution(12);
  timer1.begin(defaultAudioOutput, 1000000UL / audio_rate);
}
#endif


//-----------------------------------------------------------------------------------------------------------------
// Class functions
//-----------------------------------------------------------------------------------------------------------------

void PlatformTeensy3::start(int control_rate_hz) {
  setupMozziADC(); // you can use setupFastAnalogRead() with FASTER or FASTEST
                   // in setup() if desired (not for Teensy 3.* )
  // delay(200); // so AutoRange doesn't read 0 to start with
  startControl(control_rate_hz);
  #if (AUDIO_MODE == STANDARD) || (AUDIO_MODE == STANDARD_PLUS)
    startAudioStandard();
  #elif (AUDIO_MODE == HIFI)
    startAudioHiFi();
  #endif
}

void PlatformTeensy3::stop() {
  timer1.end();
  // ps - nointerrupts was never called so the following is not necessary: 
  // interrupts();
}

unsigned long PlatformTeensy3::audioTicks() {
  return PlatformCommon::audioTicks();
}

unsigned long PlatformTeensy3::mozziMicros() { 
  return PlatformCommon::mozziMicros();
}

void PlatformTeensy3::audioHook() // 2us on AVR excluding updateAudio()
{
  PlatformCommon::audioHook();
}

#endif  // IS_TEENSY
