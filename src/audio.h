#ifndef AUDIO_H
#define AUDIO_H

#include <SDL2/SDL.h>
#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

enum WaveformType {
    WAVEFORM_SINE = 0,
    WAVEFORM_SQUARE = 1,
    WAVEFORM_SAWTOOTH = 2,
    WAVEFORM_TRIANGLE = 3,
    WAVEFORM_NOISE = 4
};

inline const char* const WAVEFORM_NAMES[] = {
    "Sine Wave (Smooth)",
    "Square / Pulse (Classic 8-bit)",
    "Sawtooth Wave (Arcade)",
    "Triangle Wave (Retro Bass)",
    "Noise (8-bit Static)"
};
inline const int WAVEFORM_COUNT = 5;

struct AudioConfig {
    bool sound_enabled = true;      
    int waveform_type = WAVEFORM_SQUARE; 
    float waveform_freq = 440.0f;   
    float volume = 0.35f;           
    float duty_cycle = 0.50f;       
    bool beep = false;              
    bool test_tone_active = false;  

    double phase = 0.0;
    uint32_t noise_lfsr = 0xACE1u;  
};


inline void audio_callback(void* userdata, Uint8* stream, int len) {
    AudioConfig* cfg = static_cast<AudioConfig*>(userdata);
    int16_t* audio_buffer = reinterpret_cast<int16_t*>(stream);
    int samples = len / 2; 
    const double two_pi = 6.28318530717958647692;
    const double sample_rate = 44100.0;
    double phase_inc = two_pi * cfg->waveform_freq / sample_rate;
    int16_t max_amplitude = static_cast<int16_t>(cfg->volume * 16000.0f);

    bool should_play = cfg->sound_enabled && (cfg->beep || cfg->test_tone_active);

    for (int i = 0; i < samples; ++i) {
        if (should_play) {
            double sample_val = 0.0;
            switch (cfg->waveform_type) {
                case WAVEFORM_SINE:
                    sample_val = std::sin(cfg->phase);
                    break;
                case WAVEFORM_SQUARE:
                    sample_val = ((cfg->phase / two_pi) < cfg->duty_cycle) ? 1.0 : -1.0;
                    break;
                case WAVEFORM_SAWTOOTH:
                    sample_val = 2.0 * (cfg->phase / two_pi) - 1.0;
                    break;
                case WAVEFORM_TRIANGLE:
                    sample_val = 2.0 * std::fabs(2.0 * (cfg->phase / two_pi) - 1.0) - 1.0;
                    break;
                case WAVEFORM_NOISE: {
                    
                    unsigned bit = ((cfg->noise_lfsr >> 0) ^ (cfg->noise_lfsr >> 2) ^ (cfg->noise_lfsr >> 3) ^ (cfg->noise_lfsr >> 5)) & 1u;
                    cfg->noise_lfsr = (cfg->noise_lfsr >> 1) | (bit << 15);
                    sample_val = (static_cast<double>(cfg->noise_lfsr & 0xFF) / 127.5) - 1.0;
                    break;
                }
                default:
                    sample_val = std::sin(cfg->phase);
                    break;
            }

            audio_buffer[i] = static_cast<int16_t>(sample_val * max_amplitude);
            cfg->phase += phase_inc;
            if (cfg->phase >= two_pi) cfg->phase -= two_pi;
        } else {
            audio_buffer[i] = 0;
            cfg->phase = 0.0;
        }
    }
}


inline void generate_waveform_preview(const AudioConfig& cfg, float* points, int count) {
    const double two_pi = 6.28318530717958647692;
    uint32_t lfsr = 0xACE1u;
    for (int i = 0; i < count; i++) {
        double t = (static_cast<double>(i) / (count - 1)) * 2.0; 
        double cycle_pos = std::fmod(t, 1.0);
        double phase = cycle_pos * two_pi;
        double val = 0.0;
        switch (cfg.waveform_type) {
            case WAVEFORM_SINE:
                val = std::sin(phase);
                break;
            case WAVEFORM_SQUARE:
                val = (cycle_pos < cfg.duty_cycle) ? 1.0 : -1.0;
                break;
            case WAVEFORM_SAWTOOTH:
                val = 2.0 * cycle_pos - 1.0;
                break;
            case WAVEFORM_TRIANGLE:
                val = 2.0 * std::fabs(2.0 * cycle_pos - 1.0) - 1.0;
                break;
            case WAVEFORM_NOISE: {
                unsigned bit = ((lfsr >> 0) ^ (lfsr >> 2) ^ (lfsr >> 3) ^ (lfsr >> 5)) & 1u;
                lfsr = (lfsr >> 1) | (bit << 15);
                val = (static_cast<double>(lfsr & 0xFF) / 127.5) - 1.0;
                break;
            }
            default:
                val = std::sin(phase);
                break;
        }
        points[i] = static_cast<float>(val * cfg.volume);
    }
}

#endif 
