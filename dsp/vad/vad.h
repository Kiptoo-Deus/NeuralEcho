#pragma once
#include "audio_frame.h"

namespace neural_echo::dsp {

enum class VADBackend { Energy, WebRTC };

struct VADConfig {
    VADBackend backend           = VADBackend::Energy;
    float      energy_threshold  = 1e-5f;
    int        webrtc_aggressiveness = 2; ///< 0-3, higher = more aggressive
};

class VoiceActivityDetector {
public:
    explicit VoiceActivityDetector(VADConfig cfg = {}) : cfg_(cfg) {}

    /// Returns true if voice is detected in the frame.
    bool detect(const AudioFrame& frame);

    void reset() { smoothed_energy_ = 0.f; }

private:
    VADConfig cfg_;
    float     smoothed_energy_ = 0.f;
};

} // namespace neural_echo::dsp
