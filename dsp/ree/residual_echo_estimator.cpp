#include "residual_echo_estimator.h"
#include <cmath>
#include <stdexcept>
#include <algorithm>

namespace neural_echo::dsp {

ResidualEchoEstimator::ResidualEchoEstimator(REEConfig cfg) : cfg_(cfg) {
    if (cfg_.freq_bins == 0)
        throw std::invalid_argument("ResidualEchoEstimator: freq_bins must be > 0");
    mic_psd_smooth_.assign(cfg_.freq_bins, 0.f);
    echo_psd_smooth_.assign(cfg_.freq_bins, 0.f);
}

void ResidualEchoEstimator::estimate(const std::vector<float>& mic_spectrum,
                                      const std::vector<float>& echo_spectrum,
                                      std::vector<float>&       residual_out)
{
    const std::size_t K = cfg_.freq_bins;
    residual_out.resize(K);

    const float alpha  = cfg_.smoothing_alpha;
    const float beta   = 1.f - alpha;

    for (std::size_t k = 0; k < K; ++k) {
        // Power spectral density estimate (magnitude squared)
        float mic_psd  = mic_spectrum[k]  * mic_spectrum[k];
        float echo_psd = echo_spectrum[k] * echo_spectrum[k];

        // Smooth PSD estimates with exponential moving average
        mic_psd_smooth_[k]  = alpha * mic_psd_smooth_[k]  + beta * mic_psd;
        echo_psd_smooth_[k] = alpha * echo_psd_smooth_[k] + beta * echo_psd;

        // Over-subtraction: residual_psd = mic_psd - alpha * echo_psd
        float residual_psd = mic_psd_smooth_[k]
                             - cfg_.over_subtraction * echo_psd_smooth_[k];

        // Spectral floor: prevent negative residual
        float floor_psd = cfg_.spectral_floor * mic_psd_smooth_[k];
        residual_psd = std::max(residual_psd, floor_psd);

        // Convert PSD back to magnitude
        residual_out[k] = std::sqrt(residual_psd);
    }
    initialised_ = true;
}

void ResidualEchoEstimator::reset() {
    std::fill(mic_psd_smooth_.begin(),  mic_psd_smooth_.end(),  0.f);
    std::fill(echo_psd_smooth_.begin(), echo_psd_smooth_.end(), 0.f);
    initialised_ = false;
}

} // namespace neural_echo::dsp
