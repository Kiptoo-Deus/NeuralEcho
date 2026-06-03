#include "vad.h"
#include <cmath>

namespace neural_echo::dsp {

bool VoiceActivityDetector::detect(const AudioFrame& frame) {
    float energy = 0.f;
    for (std::size_t i = 0; i < frame.length; ++i)
        energy += frame.data[i] * frame.data[i];
    energy /= static_cast<float>(frame.length + 1);

    smoothed_energy_ = 0.95f * smoothed_energy_ + 0.05f * energy;

    return smoothed_energy_ > cfg_.energy_threshold;
}

} // namespace neural_echo::dsp
