#pragma once

// Preconditions for Platform_config.h - Forward declaration of implementation calsses
class PlatformMBED;
class PlatformAVR;
class PlatformESP32_I2S;
class PlatformESP8266;
class PlatformCommon;
class PlatformSAMD21;
class PlatformSTM32;
class PlatformTeensy3;
class PlatformRP2040;
class PlatformRP2040_I2S;

//enum audio_modes {STANDARD,STANDARD_PLUS,HIFI};
#define STANDARD 0
#define STANDARD_PLUS 1
#define HIFI 2

#define OUTPUT_MONO 1
#define OUTPUT_STEREO 2
enum audio_channels {MONO=OUTPUT_MONO, STEREO=OUTPUT_STEREO};

