#include "aec_pipeline.h"
#include <chrono>
#include <cmath>
#include <stdexcept>

namespace neural_echo::dsp {

AECPipeline::AECPipeline(AECConfig cfg)
    : cfg_(cfg),
      fft_(cfg.fft_size, cfg.hop_size),
      nlms_(cfg.nlms),
      dtd_(cfg.dtd),
      agc_(cfg.agc),
      vad_(cfg.vad)
{
    if (cfg_.frame_size == 0)
        throw std::invalid_argument("AECPipeline: frame_size must be > 0");
    ola_buffer_.resize(cfg_.frame_size);
}

AECPipeline::~AECPipeline() = default;

void AECPipeline::reset() {
    nlms_.reset();
    dtd_.reset();
    agc_.reset();
    vad_.reset();
    metrics_.reset();
    ola_buffer_.resize(cfg_.frame_size, 0.f);
}

AudioBuffer AECPipeline::process(const AudioFrame& far_end, const AudioFrame& mic) {
    if (far_end.length != cfg_.frame_size || mic.length != cfg_.frame_size)
        throw std::invalid_argument("AECPipeline: frame length mismatch");

    auto t_start = std::chrono::steady_clock::now();

    // 1. Double-talk detection
    bool adapt_frozen = false;
    dtd_.detect(far_end, mic, adapt_frozen);
    if (adapt_frozen) nlms_.freeze(); else nlms_.resume();

    // 2. Adaptive filtering
    EchoEstimate est = nlms_.process_frame(far_end, mic);

    // 3. Apply AGC to output
    agc_.process(est.time_domain);

    // 4. Compute metrics
    auto t_end = std::chrono::steady_clock::now();
    double proc_us = std::chrono::duration<double, std::micro>(t_end - t_start).count();
    double frame_us = static_cast<double>(cfg_.frame_size)
                      / static_cast<double>(cfg_.sample_rate) * 1e6;

    // ERLE estimate (simplified: power ratio in dB)
    float mic_pwr = 0.f, out_pwr = 0.f;
    for (std::size_t i = 0; i < cfg_.frame_size; ++i) {
        mic_pwr += mic.data[i] * mic.data[i];
        out_pwr += est.time_domain.samples[i] * est.time_domain.samples[i];
    }
    float erle_db = 10.f * std::log10((mic_pwr + 1e-9f) / (out_pwr + 1e-9f));

    FrameMetrics fm;
    fm.erle_db      = erle_db;
    fm.mic_power_db = 10.f * std::log10(mic_pwr / cfg_.frame_size + 1e-9f);
    fm.out_power_db = 10.f * std::log10(out_pwr / cfg_.frame_size + 1e-9f);
    fm.rtf          = static_cast<float>(proc_us / frame_us);
    fm.timestamp_ns = mic.timestamp_ns;

    metrics_.push(fm);

    if (callback_)
        callback_(est.time_domain, fm);

    return est.time_domain;
}

} // namespace neural_echo::dsp
