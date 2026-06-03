#pragma once
#include "audio_frame.h"

namespace neural_echo::dsp {

struct AGCConfig {
    float target_rms      = 0.1f;    ///< Desired RMS output level
    float attack_coeff    = 0.99f;   ///< Fast envelope follower
    float release_coeff   = 0.9999f; ///< Slow release
    float max_gain        = 30.f;
    float min_gain        = 0.01f;
};

class AutomaticGainControl {
public:
    explicit AutomaticGainControl(AGCConfig cfg = {}) : cfg_(cfg) {}

    void process(AudioBuffer& buf);
    void reset()  { gain_ = 1.f; envelope_ = 0.f; }

private:
    AGCConfig cfg_;
    float     gain_     = 1.f;
    float     envelope_ = 0.f;
};

} // namespace neural_echo::dsp
