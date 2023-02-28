#pragma once
#include "CircularBuffer.h"
#include "Mozzi.h"

extern AudioOutput_t updateAudio();
extern void updateControl();

#if EXTERNAL_AUDIO_OUTPUT == true
extern void audioOutput(const AudioOutput value);
#endif

/**
 * @brief Root of all new output Classes
 * We do not use any virtual functions because of the performance impact. Instead we impelmented the shared functionality in 
 * the PlatformCommon class
 */
class PlatformOutput {
};


/**
* Public Shared Data 
*/
static uint16_t update_control_timeout;
static uint16_t update_control_counter;
static CircularBuffer<AudioOutput_t> output_buffer;

#if USE_AUDIO_INPUT == true
static AudioOutputStorage_t common_audio_input = 0;
static CircularBuffer<AudioOutput_t> input_buffer;  
#endif

/**
* @brief Shared functionality between all new Mozzi Platform implementations.
* Please do not use any platform specific #IFDEF here - The right place to implement
* Platform specific functionality is the related platform implementation class!
*/
class PlatformCommon {
    public:
        PlatformCommon() {
        }

        const int channels() const {
            return AUDIO_CHANNELS;
        }

        static inline bool advanceControlLoop() {
            bool result = false;
            if (!update_control_counter) {
                update_control_counter = update_control_timeout;
                updateControl();
                result = true;
            } else {
                --update_control_counter;
            }
            return result;
        }

        void startControl(unsigned int control_rate_hz) {
            update_control_timeout = AUDIO_RATE / control_rate_hz;
        }

        static unsigned long audioTicks() {
            return output_buffer.count();
        }

        static unsigned long mozziMicros() { 
            return audioTicks() * MICROS_PER_AUDIO_TICK;
        }

        static void audioHook()  {
            #if (USE_AUDIO_INPUT == true) 
            updateAudioInput();
            #endif

            if (canBufferAudioOutput()) {
                advanceControlLoop();
                bufferAudioOutput(updateAudio());
            }
        }

        static bool canBufferAudioOutput(){
            return !output_buffer.isFull();
        } 

        static void bufferAudioOutput(const AudioOutput_t f){
            output_buffer.write(f);
        }

        bool write(const AudioOutput out){
            if (canBufferAudioOutput() ){
                bufferAudioOutput(out);
                return true;
            } else {
                return false;
            }
        }


#if USE_AUDIO_INPUT == true

        static void updateAudioInput(){
            if (!input_buffer.isEmpty()){
                common_audio_input = input_buffer.read();
            }
        }

        static void setAudioInput(int value){
          if (!input_buffer.isFull()) {
            input_buffer.write(value);
          }
        }

        static int getAudioInput() { 
            return common_audio_input;
        }
#endif


};

