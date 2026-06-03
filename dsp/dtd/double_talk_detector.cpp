#include "double_talk_detector.h"
#include <numeric>
#include <cmath>

namespace neural_echo::dsp {

static float frame_power(const AudioFrame& f) {
    float s = 0.f;
    for (std::size_t i = 0; i < f.length; ++i)
        s += f.data[i] * f.data[i];
    return s / static_cast<float>(f.length + 1);
}

TalkState DoubleTalkDetector::detect(const AudioFrame& far_end,
                                      const AudioFrame& mic,
                                      bool& adapt_frozen)
{
    float far_p = frame_power(far_end);
    float mic_p = frame_power(mic);

    // Smooth power estimates
    far_power_smooth_ = cfg_.smoothing_alpha * far_power_smooth_
                        + (1.f - cfg_.smoothing_alpha) * far_p;
    mic_power_smooth_ = cfg_.smoothing_alpha * mic_power_smooth_
                        + (1.f - cfg_.smoothing_alpha) * mic_p;

    constexpr float kSilenceThreshold = 1e-8f;
    bool far_active = far_power_smooth_ > kSilenceThreshold;
    bool mic_active = mic_power_smooth_ > kSilenceThreshold;

    TalkState state = TalkState::Silence;
    if (far_active && mic_active) {
        // Geigel criterion: if mic power is high relative to far-end, double-talk
        float ratio = mic_power_smooth_ / (far_power_smooth_ + 1e-9f);
        state = (ratio > cfg_.energy_ratio_threshold)
                    ? TalkState::DoubleTalk
                    : TalkState::FarEndOnly;
    } else if (far_active) {
        state = TalkState::FarEndOnly;
    } else if (mic_active) {
        state = TalkState::NearEndOnly;
    }

    adapt_frozen = (state == TalkState::DoubleTalk ||
                    state == TalkState::NearEndOnly);
    return state;
}

void DoubleTalkDetector::reset() {
    far_power_smooth_ = 0.f;
    mic_power_smooth_ = 0.f;
}

} // namespace neural_echo::dsp
