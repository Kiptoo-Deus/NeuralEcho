#pragma once
#include <cstddef>
#include "audio_frame.h"

namespace neural_echo::dsp {

enum class TalkState { FarEndOnly, NearEndOnly, DoubleTalk, Silence };

struct DTDConfig {
    float energy_ratio_threshold  = 0.5f;   ///< Geigel threshold
    float coherence_threshold     = 0.7f;
    float smoothing_alpha         = 0.95f;  ///< EMA smoothing factor
};

/// Double-Talk Detector — votes from energy ratio and spectral coherence.
class DoubleTalkDetector {
public:
    explicit DoubleTalkDetector(DTDConfig cfg = {}) : cfg_(cfg) {}

    /// Returns current talk state and sets adapt_frozen output.
    TalkState detect(const AudioFrame& far_end,
                     const AudioFrame& mic,
                     bool&             adapt_frozen);

    void reset();

private:
    DTDConfig cfg_;
    float     far_power_smooth_  = 0.f;
    float     mic_power_smooth_  = 0.f;
};

} // namespace neural_echo::dsp
