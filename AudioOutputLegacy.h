#pragma once

///////////////////// SAMD21
#if IS_SAMD21()
#include "AudioConfigSAMD21.h"
inline void audioOutput(const AudioOutput f)
{
  analogWrite(AUDIO_CHANNEL_1_PIN, f.l()+AUDIO_BIAS);
}
#endif


///////////////////// TEENSY3
#if IS_TEENSY3()
#include "AudioConfigTeensy3_12bit.h"
inline void audioOutput(const AudioOutput f)
{
  analogWrite(AUDIO_CHANNEL_1_PIN, f.l()+AUDIO_BIAS);
}
#endif


///////////////////// STM32
#if IS_STM32()
#include "AudioConfigSTM32.h"
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


///////////////////// ESP8266
#if IS_ESP8266()
#include "AudioConfigESP.h"
#if (ESP_AUDIO_OUT_MODE == PDM_VIA_I2S)
#include <i2s.h>
inline bool canBufferAudioOutput() {
  return (i2s_available() >= PDM_RESOLUTION);
}
inline void audioOutput(const AudioOutput f) {
  for (uint8_t words = 0; words < PDM_RESOLUTION; ++words) {
    i2s_write_sample(pdmCode32(f.l()));
  }
}
#elif (ESP_AUDIO_OUT_MODE == EXTERNAL_DAC_VIA_I2S)
#include <i2s.h>
inline bool canBufferAudioOutput() {
  return (i2s_available() >= PDM_RESOLUTION);
}
inline void audioOutput(const AudioOutput f) {
  i2s_write_lr(f.l(), f.r());  // Note: i2s_write expects zero-centered output
}
#else
inline void audioOutput(const AudioOutput f) {
  // optimized version of: Serial1.write(...);
  for (uint8_t i = 0; i < PDM_RESOLUTION*4; ++i) {
    U1F = pdmCode8(f);
  }
}
#endif
#endif



///////////////////// ESP32
#if IS_ESP32()
#include "AudioConfigESP32.h"
// On ESP32 we cannot test wether the DMA buffer has room. Instead, we have to use a one-sample mini buffer. In each iteration we
// _try_ to write that sample to the DMA buffer, and if successful, we can buffer the next sample. Somewhat cumbersome, but works.
// TODO: Should ESP32 gain an implemenation of i2s_available(), we should switch to using that, instead.
static bool _esp32_can_buffer_next = true;
#if (ESP32_AUDIO_OUT_MODE == INTERNAL_DAC)
static uint16_t _esp32_prev_sample[2];
#define ESP_SAMPLE_SIZE (2*sizeof(uint16_t))
#elif (ESP32_AUDIO_OUT_MODE == PT8211_DAC)
static int16_t _esp32_prev_sample[2];
#define ESP_SAMPLE_SIZE (2*sizeof(int16_t))
#elif (ESP32_AUDIO_OUT_MODE == PDM_VIA_I2S)
static uint32_t _esp32_prev_sample[PDM_RESOLUTION];
#define ESP_SAMPLE_SIZE (PDM_RESOLUTION*sizeof(uint32_t))
#endif

inline bool esp32_tryWriteSample() {
  size_t bytes_written;
  i2s_write(i2s_num, &_esp32_prev_sample, ESP_SAMPLE_SIZE, &bytes_written, 0);
  return (bytes_written != 0);
}

inline bool canBufferAudioOutput() {
  if (_esp32_can_buffer_next) return true;
  _esp32_can_buffer_next = esp32_tryWriteSample();
  return _esp32_can_buffer_next;
}

inline void audioOutput(const AudioOutput f) {
#if (ESP32_AUDIO_OUT_MODE == INTERNAL_DAC)
  _esp32_prev_sample[0] = (f.l() + AUDIO_BIAS) << 8;
#if (AUDIO_CHANNELS > 1)
  _esp32_prev_sample[1] = (f.r() + AUDIO_BIAS) << 8;
#else
  // For simplicity of code, even in mono, we're writing stereo samples
  _esp32_prev_sample[1] = _esp32_prev_sample[0];
#endif
#elif (ESP32_AUDIO_OUT_MODE == PDM_VIA_I2S)
  for (uint8_t i=0; i<PDM_RESOLUTION; ++i) {
    _esp32_prev_sample[i] = pdmCode32(f.l() + AUDIO_BIAS);
  }
#else
  // PT8211 takes signed samples
  _esp32_prev_sample[0] = f.l();
  _esp32_prev_sample[1] = f.r();
#endif
  _esp32_can_buffer_next = esp32_tryWriteSample();
}

#endif



///////////////////// AVR STANDARD
#if IS_AVR() && (AUDIO_MODE == STANDARD_PLUS)
#include "AudioConfigStandardPlus.h"
inline void audioOutput(const AudioOutput f)
{
  AUDIO_CHANNEL_1_OUTPUT_REGISTER = f.l()+AUDIO_BIAS;
#if (AUDIO_CHANNELS > 1)
  AUDIO_CHANNEL_2_OUTPUT_REGISTER = f.r()+AUDIO_BIAS;
#endif
}



///////////////////// AVR HIFI
#elif IS_AVR() && (AUDIO_MODE == HIFI)
#include "AudioConfigHiSpeed14bitPwm.h"
inline void audioOutput(const AudioOutput f)
{
  // read about dual pwm at
  // http://www.openmusiclabs.com/learning/digital/pwm-dac/dual-pwm-circuits/
  // sketches at http://wiki.openmusiclabs.com/wiki/PWMDAC,
  // http://wiki.openmusiclabs.com/wiki/MiniArDSP
  // if (!output_buffer.isEmpty()){
  //unsigned int out = output_buffer.read();
  // 14 bit, 7 bits on each pin
  // AUDIO_CHANNEL_1_highByte_REGISTER = out >> 7; // B00111111 10000000 becomes
  // B1111111
  // try to avoid looping over 7 shifts - need to check timing or disassemble to
  // see what really happens unsigned int out_high = out<<1; // B00111111
  // 10000000 becomes B01111111 00000000
  // AUDIO_CHANNEL_1_highByte_REGISTER = out_high >> 8; // B01111111 00000000
  // produces B01111111 AUDIO_CHANNEL_1_lowByte_REGISTER = out & 127;
  /* Atmega manual, p123
  The high byte (OCR1xH) has to be written first.
  When the high byte I/O location is written by the CPU,
  the TEMP Register will be updated by the value written.
  Then when the low byte (OCR1xL) is written to the lower eight bits,
  the high byte will be copied into the upper 8-bits of
  either the OCR1x buffer or OCR1x Compare Register in
  the same system clock cycle.
  */
  AUDIO_CHANNEL_1_highByte_REGISTER = (f.l()+AUDIO_BIAS) >> AUDIO_BITS_PER_REGISTER;
  AUDIO_CHANNEL_1_lowByte_REGISTER = (f.l()+AUDIO_BIAS) & ((1 << AUDIO_BITS_PER_REGISTER) - 1);
}
#endif
