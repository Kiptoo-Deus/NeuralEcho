#pragma once
#include <cstddef>
#include <vector>
#include "audio_frame.h"

namespace neural_echo::dsp {

struct NLMSConfig {
    std::size_t filter_length  = 512;   ///< Number of taps
    float       step_size      = 0.5f;  ///< Initial mu (0, 2)
    float       leakage        = 1e-5f; ///< Weight leakage factor
    float       regularisation = 1e-6f; ///< Denominator stabiliser
    bool        enable_simd    = true;
};

/// Normalised Least Mean Squares adaptive echo canceller.
///
/// Processes one sample at a time. Call process_frame() to drive the filter
/// with a full audio frame at once.
class NLMSFilter {
public:
    explicit NLMSFilter(NLMSConfig cfg = {});

    // Non-copyable
    NLMSFilter(const NLMSFilter&)            = delete;
    NLMSFilter& operator=(const NLMSFilter&) = delete;
    NLMSFilter(NLMSFilter&&)                 = default;
    NLMSFilter& operator=(NLMSFilter&&)      = default;

    /// Process a full frame pair.
    /// @param far_end   Loudspeaker reference signal (x[n])
    /// @param mic       Microphone signal (d[n])
    /// @returns Echo estimate e[n] = d[n] - x^T * w
    EchoEstimate process_frame(const AudioFrame& far_end,
                                const AudioFrame& mic);

    /// Freeze weight adaptation (e.g., during double-talk).
    void freeze()  noexcept { frozen_ = true;  }
    void resume()  noexcept { frozen_ = false; }
    bool frozen()  const noexcept { return frozen_; }

    /// Reset filter weights and delay line to zero.
    void reset();

    const NLMSConfig& config() const { return cfg_; }

private:
    float process_sample(float x, float d);

    NLMSConfig          cfg_;
    std::vector<float>  weights_;     ///< Filter taps w[n]
    std::vector<float>  delay_line_;  ///< Circular buffer of far-end samples
    std::size_t         delay_idx_ = 0;
    bool                frozen_    = false;

    // Temporary frame-level buffers
    AudioBuffer         output_buffer_;
};

} // namespace neural_echo::dsp
