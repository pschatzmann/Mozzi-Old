/*
 * MozziSamd21.cpp
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
#if IS_SAMD21() && USE_LEGACY_GUTS == false

#include "PlatformSAMD21.h"

PlatformCommon Common;
static uint16_t audio_rate = AUDIO_RATE;

void PlatformSAMD21::setAudioRate(uint16_t rate){
    audio_rate = rate;
}

bool PlatformSAMD21::write(const AudioOutput out){
  return Common.write(out);
}

bool PlatformSAMD21::canWrite(){
  return Common.canBufferAudioOutput();
}

#if (EXTERNAL_AUDIO_OUTPUT != true)

inline void audioOutput(const AudioOutput f)
{
  analogWrite(AUDIO_CHANNEL_1_PIN, f.l()+AUDIO_BIAS);
}

#endif

//-----------------------------------------------------------------------------------------------------------------
////// BEGIN AUDIO INPUT code ////////
//-----------------------------------------------------------------------------------------------------------------
#if (USE_AUDIO_INPUT == true)

uint8_t adc_count = 0;

int PlatformSAMD21::getAudioInput() { return PlatformCommon::getAudioInput(); }

static void startFirstAudioADC() {
  adcStartConversion(adcPinToChannelNum(AUDIO_INPUT_PIN));
}

static void startSecondAudioADC() {
  ADCSRA |= (1 << ADSC); // start a second conversion on the current channel
}

static void receiveSecondAudioADC() {
  Common.setAudioInput(ADC)
}

#endif
//-----------------------------------------------------------------------------------------------------------------
////// Control
//-----------------------------------------------------------------------------------------------------------------


// These are ARM SAMD21 Timer 5 routines to establish a sample rate interrupt
static bool tcIsSyncing() {
  return TC5->COUNT16.STATUS.reg & TC_STATUS_SYNCBUSY;
}

static void tcReset() {
  // Reset TCx
  TC5->COUNT16.CTRLA.reg = TC_CTRLA_SWRST;
  while (tcIsSyncing())
    ;
  while (TC5->COUNT16.CTRLA.bit.SWRST)
    ;
}
/* Not currently used, and does not compile with EXTERNAL_AUDIO_OUTPUT
static void tcEnd() {
  // Disable TC5
  TC5->COUNT16.CTRLA.reg &= ~TC_CTRLA_ENABLE;
  while (tcIsSyncing())
    ;
  tcReset();
  analogWrite(AUDIO_CHANNEL_1_PIN, 0);
} */

static void tcConfigure(uint32_t sampleRate) {
  // Enable GCLK for TCC2 and TC5 (timer counter input clock)
  GCLK->CLKCTRL.reg = (uint16_t)(GCLK_CLKCTRL_CLKEN | GCLK_CLKCTRL_GEN_GCLK0 |
                                 GCLK_CLKCTRL_ID(GCM_TC4_TC5));
  while (GCLK->STATUS.bit.SYNCBUSY)
    ;

  tcReset();

  // Set Timer counter Mode to 16 bits
  TC5->COUNT16.CTRLA.reg |= TC_CTRLA_MODE_COUNT16;

  // Set TC5 mode as match frequency
  TC5->COUNT16.CTRLA.reg |= TC_CTRLA_WAVEGEN_MFRQ;

  TC5->COUNT16.CTRLA.reg |= TC_CTRLA_PRESCALER_DIV1 | TC_CTRLA_ENABLE;

  TC5->COUNT16.CC[0].reg = (uint16_t)(SystemCoreClock / sampleRate - 1);
  while (tcIsSyncing())
    ;

  // Configure interrupt request
  NVIC_DisableIRQ(TC5_IRQn);
  NVIC_ClearPendingIRQ(TC5_IRQn);
  NVIC_SetPriority(TC5_IRQn, 0);
  NVIC_EnableIRQ(TC5_IRQn);

  // Enable the TC5 interrupt request
  TC5->COUNT16.INTENSET.bit.MC0 = 1;
  while (tcIsSyncing())
    ;
}

bool canBufferAudioOutput(){
  return Common.canBufferAudioOutput();
} 

void bufferAudioOutput(const AudioOutput_t f){
  Common.bufferAudioOutput(f);
}

void TC5_Handler(void) __attribute__((weak, alias("samd21AudioOutput")));

static void CACHED_FUNCTION_ATTR defaultAudioOutput() {
#if (USE_AUDIO_INPUT == true)
  adc_count = 0;
  startSecondAudioADC();
#endif
  audioOutput(output_buffer.read());
}

#if (AUDIO_MODE == STANDARD) || (AUDIO_MODE == STANDARD_PLUS) 

#ifdef __cplusplus
extern "C" {
#endif
void samd21AudioOutput() {
  defaultAudioOutput();
  TC5->COUNT16.INTFLAG.bit.MC0 = 1;
}
#ifdef __cplusplus
}
#endif

static void startAudioStandard() {

#ifdef ARDUINO_SAMD_CIRCUITPLAYGROUND_EXPRESS
  {
    static const int CPLAY_SPEAKER_SHUTDOWN = 11;
    pinMode(CPLAY_SPEAKER_SHUTDOWN, OUTPUT);
    digitalWrite(CPLAY_SPEAKER_SHUTDOWN, HIGH);
  }

#endif
  analogWriteResolution(10);
#if (EXTERNAL_AUDIO_OUTPUT != true)
  analogWrite(AUDIO_CHANNEL_1_PIN, 0);
#endif
  tcConfigure(audio_rate);

}
#endif // AUDIO_MODE


//-----------------------------------------------------------------------------------------------------------------

static void startControl(unsigned int control_rate_hz) {
  update_control_timeout = audio_rate / control_rate_hz;
}

void PlatformSAMD21::start(int control_rate_hz) {
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

void PlatformSAMD21::stop() {
  // ps - nointerrupts was never called so the following is not necessary: 
  // interrupts();
}

unsigned long PlatformSAMD21::audioTicks() {
  return PlatformCommon::audioTicks();
}

unsigned long PlatformSAMD21::mozziMicros() { 
  return PlatformCommon::mozziMicros();
}

void PlatformSAMD21::audioHook() {
  PlatformCommon::audioHook();
}

#endif  // IS_SAMD21()
