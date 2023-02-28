#ifndef AUDIOCONFIGLINUX_H
#define AUDIOCONFIGLINUX_H
#include "mozzi_config_defines.h"
/**
 * @brief Configuration for output to Linux
 */

// We do not want to use the default -> we switch to 2 bytes instead!
#define AudioOutputStorage_t int16_t
// Number of output / input bits
#define AUDIO_BITS 16
// Used internally to put the 0-biased generated audio into the centre of the output range => 0-based is OK
#define AUDIO_BIAS ((uint16_t) 0)

// Sample frequency 
#ifndef AUDIO_RATE_PLATFORM_DEFAULT
#define AUDIO_RATE_PLATFORM_DEFAULT 32768
#endif

// Default output implementation class
#ifndef PLATFORM_OUTPUT_CLASS
#define PLATFORM_OUTPUT_CLASS PlatformLinuxPulseAudio
#endif

#endif