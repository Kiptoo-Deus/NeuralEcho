#include "aec_pipeline.h"
#include <chrono>
#include <cmath>
#include <stdexcept>
#include <algorithm>

namespace neural_echo::dsp {

// ── Construction ───────────────────────────────────────────────────────────────

AECPipeline::AECPipeline(AECConfig cfg)
    : cfg_(cfg),
      fft_(cfg.fft_size, cfg.hop_size),
      nlms_(cfg.nlms),
      dtd_(cfg.dtd),
      ree_(cfg.ree),
      agc_(cfg.agc),
      vad_(cfg.vad),
      neural_(cfg.neural_config_path, cfg.enable_neural, cfg.fft_size / 2 + 1)
{
    if (cfg_.frame_size == 0)
        throw std::invalid_argument("AECPipeline: frame_size must be > 0");

    const std::size_t bins = cfg_.fft_size / 2 + 1;
    ola_accum_.assign(cfg_.fft_size, 0.f);
    far_mag_.resize(bins);
    mic_mag_.resize(bins);
    echo_mag_.resize(bins);
    residual_mag_.resize(bins);
    mask_buf_.assign(bins, 1.f);
}

AECPipeline::~AECPipeline() = default;

// ── Reset ──────────────────────────────────────────────────────────────────────

void AECPipeline::reset() {
    nlms_.reset();
    dtd_.reset();
    ree_.reset();
    agc_.reset();
    vad_.reset();
    metrics_.reset();
    std::fill(ola_accum_.begin(), ola_accum_.end(), 0.f);
    std::fill(mask_buf_.begin(),  mask_buf_.end(),  1.f);
}

// ── Per-frame processing ───────────────────────────────────────────────────────

AudioBuffer AECPipeline::process(const AudioFrame& far_end, const AudioFrame& mic) {
    if (far_end.length != cfg_.frame_size || mic.length != cfg_.frame_size)
        throw std::invalid_argument("AECPipeline: frame length mismatch");

    auto t_start = std::chrono::steady_clock::now();

    const std::size_t N    = cfg_.frame_size;
    const std::size_t FFTN = cfg_.fft_size;
    const std::size_t hop  = cfg_.hop_size;
    const std::size_t bins = FFTN / 2 + 1;

    // ── 1. Double-talk detection ───────────────────────────────────────────────
    bool adapt_frozen = false;
    dtd_.detect(far_end, mic, adapt_frozen);
    if (adapt_frozen) nlms_.freeze(); else nlms_.resume();

    // ── 2. NLMS adaptive filtering ────────────────────────────────────────────
    EchoEstimate est = nlms_.process_frame(far_end, mic);
    const AudioBuffer& error_buf = est.time_domain;  // mic - echo_estimate

    // ── 3. Compute spectra for REE + neural ───────────────────────────────────
    // Zero-pad or crop to fft_size
    std::vector<float> far_padded(FFTN, 0.f);
    std::vector<float> mic_padded(FFTN, 0.f);
    std::vector<float> err_padded(FFTN, 0.f);

    std::copy_n(far_end.data, std::min(N, FFTN), far_padded.data());
    std::copy_n(mic.data,     std::min(N, FFTN), mic_padded.data());
    std::copy_n(error_buf.samples.data(), std::min(N, FFTN), err_padded.data());

    AudioFrame far_frame{far_padded.data(), FFTN, far_end.timestamp_ns};
    AudioFrame mic_frame{mic_padded.data(), FFTN, mic.timestamp_ns};
    AudioFrame err_frame{err_padded.data(), FFTN, mic.timestamp_ns};

    ComplexSpectrum far_spec  = fft_.stft(far_frame);
    ComplexSpectrum mic_spec  = fft_.stft(mic_frame);
    ComplexSpectrum err_spec  = fft_.stft(err_frame);

    FFTEngine::magnitude(far_spec,  far_mag_);
    FFTEngine::magnitude(mic_spec,  mic_mag_);
    FFTEngine::magnitude(err_spec,  echo_mag_);

    // ── 4. Residual echo estimation ───────────────────────────────────────────
    ree_.estimate(mic_mag_, echo_mag_, residual_mag_);

    // ── 5. Neural inference — suppression mask ────────────────────────────────
    neural_.process_frame(far_mag_.data(), mic_mag_.data(),
                          residual_mag_.data(), mask_buf_.data());

    // ── 6. Apply spectral mask to error signal spectrum ───────────────────────
    NeuralBridge::apply_mask(mask_buf_.data(), err_spec, bins);

    // ── 7. ISTFT → overlap-add reconstruction ─────────────────────────────────
    AudioBuffer istft_out = fft_.istft(err_spec);   // length = fft_size

    // OLA: add to accumulator and extract frame
    for (std::size_t i = 0; i < FFTN; ++i)
        ola_accum_[i] += istft_out.samples[i];

    // Output first N samples
    AudioBuffer output;
    output.samples.resize(N);
    output.timestamp_ns = mic.timestamp_ns;
    std::copy_n(ola_accum_.data(), N, output.samples.data());

    // Shift OLA buffer left by N (hop)
    const std::size_t shift = std::min(N, FFTN);
    std::move(ola_accum_.data() + shift,
              ola_accum_.data() + FFTN,
              ola_accum_.data());
    std::fill(ola_accum_.data() + (FFTN - shift), ola_accum_.data() + FFTN, 0.f);

    // ── 8. AGC ────────────────────────────────────────────────────────────────
    agc_.process(output);

    // ── 9. Metrics ────────────────────────────────────────────────────────────
    auto t_end = std::chrono::steady_clock::now();
    double proc_us  = std::chrono::duration<double, std::micro>(t_end - t_start).count();
    double frame_us = static_cast<double>(N) / static_cast<double>(cfg_.sample_rate) * 1e6;

    float mic_pwr = 0.f, out_pwr = 0.f;
    for (std::size_t i = 0; i < N; ++i) {
        mic_pwr += mic.data[i]          * mic.data[i];
        out_pwr += output.samples[i]    * output.samples[i];
    }
    float erle_db = 10.f * std::log10((mic_pwr + 1e-9f) / (out_pwr + 1e-9f));

    FrameMetrics fm;
    fm.erle_db      = erle_db;
    fm.mic_power_db = 10.f * std::log10(mic_pwr / static_cast<float>(N) + 1e-9f);
    fm.out_power_db = 10.f * std::log10(out_pwr / static_cast<float>(N) + 1e-9f);
    fm.rtf          = static_cast<float>(proc_us / frame_us);
    fm.timestamp_ns = mic.timestamp_ns;
    metrics_.push(fm);

    if (callback_) callback_(output, fm);

    return output;
}

} // namespace neural_echo::dsp
