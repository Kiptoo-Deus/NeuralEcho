#include "agc.h"
#include <cmath>
#include <algorithm>

namespace neural_echo::dsp {

void AutomaticGainControl::process(AudioBuffer& buf) {
    for (auto& s : buf.samples) {
        float abs_s = std::fabs(s);
        // Envelope follower
        float coeff  = (abs_s > envelope_) ? cfg_.attack_coeff
                                           : cfg_.release_coeff;
        envelope_ = coeff * envelope_ + (1.f - coeff) * abs_s;

        // Compute desired gain
        float desired = (envelope_ > 1e-9f)
                        ? cfg_.target_rms / envelope_
                        : cfg_.max_gain;
        desired = std::clamp(desired, cfg_.min_gain, cfg_.max_gain);

        // Smooth gain transitions
        gain_ = 0.999f * gain_ + 0.001f * desired;
        s    *= gain_;
    }
}

} // namespace neural_echo::dsp
