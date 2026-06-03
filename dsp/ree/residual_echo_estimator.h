#pragma once
#include <vector>
#include <cstddef>
#include "../audio_frame.h"

namespace neural_echo::dsp {

struct REEConfig {
    std::size_t freq_bins        = 257;   ///< fft_size/2 + 1
    float       over_subtraction = 2.0f;  ///< Alpha: over-subtraction factor
    float       spectral_floor   = 0.01f; ///< Beta:  minimum spectral floor
    float       smoothing_alpha  = 0.85f; ///< EMA smoothing for PSD estimates
};

/// Residual Echo Estimator.
///
/// Estimates the power spectral density of uncancelled echo remaining after
/// the adaptive filter. Uses over-subtraction with a spectral floor to prevent
/// negative values. Output feeds directly into the neural suppressor as the
/// residual_spectrum input feature.
class ResidualEchoEstimator {
public:
    explicit ResidualEchoEstimator(REEConfig cfg = {});

    /// Update PSD estimates and compute residual magnitude spectrum.
    ///
    /// @param mic_spectrum   Magnitude spectrum of microphone signal  [freq_bins]
    /// @param echo_spectrum  Magnitude spectrum of echo estimate      [freq_bins]
    /// @param residual_out   Output residual magnitude spectrum       [freq_bins]
    void estimate(const std::vector<float>& mic_spectrum,
                  const std::vector<float>& echo_spectrum,
                  std::vector<float>&       residual_out);

    void reset();

    const REEConfig& config() const { return cfg_; }

private:
    REEConfig          cfg_;
    std::vector<float> mic_psd_smooth_;
    std::vector<float> echo_psd_smooth_;
    bool               initialised_ = false;
};

} // namespace neural_echo::dsp
