#pragma once
#include <functional>
#include <memory>
#include <string>
#include "audio_frame.h"
#include "fft/fft_engine.h"
#include "nlms/nlms_filter.h"
#include "dtd/double_talk_detector.h"
#include "ree/residual_echo_estimator.h"
#include "agc/agc.h"
#include "vad/vad.h"
#include "metrics/metrics_collector.h"
#include "neural_bridge.h"

namespace neural_echo::dsp {

struct AECConfig {
    std::size_t sample_rate    = 16000;
    std::size_t frame_size     = 160;    ///< 10 ms @ 16 kHz
    std::size_t fft_size       = 512;
    std::size_t hop_size       = 128;
    NLMSConfig  nlms;
    DTDConfig   dtd;
    REEConfig   ree;
    AGCConfig   agc;
    VADConfig   vad;
    bool        enable_neural  = true;
    std::string neural_config_path;     ///< Path to neural_echo.toml; empty = defaults
};

using FrameCallback = std::function<void(const AudioBuffer&, const FrameMetrics&)>;

/// Top-level AEC pipeline facade.
///
/// Processing chain per frame:
///   1. Double-Talk Detection  → freeze/resume NLMS adaptation
///   2. NLMS Adaptive Filter   → initial echo estimate
///   3. FFT of error signal    → spectral domain processing
///   4. Residual Echo Estimator→ residual PSD
///   5. NeuralBridge           → suppression mask (ONNX inference)
///   6. Spectral masking       → apply mask to STFT
///   7. ISTFT + overlap-add    → time-domain output
///   8. AGC                    → output level normalisation
///   9. MetricsCollector       → ERLE, RTF update
class AECPipeline {
public:
    explicit AECPipeline(AECConfig cfg = {});
    ~AECPipeline();

    AECPipeline(const AECPipeline&)            = delete;
    AECPipeline& operator=(const AECPipeline&) = delete;
    AECPipeline(AECPipeline&&)                 = default;

    /// Process one pair of frames.
    /// @param far_end  Loudspeaker reference (length == cfg.frame_size)
    /// @param mic      Microphone signal     (length == cfg.frame_size)
    /// @returns        Echo-cancelled output (length == cfg.frame_size)
    AudioBuffer process(const AudioFrame& far_end, const AudioFrame& mic);

    void set_frame_callback(FrameCallback cb) { callback_ = std::move(cb); }

    const AECConfig&        config()  const { return cfg_; }
    const MetricsCollector& metrics() const { return metrics_; }

    void reset();

private:
    AECConfig             cfg_;
    FFTEngine             fft_;
    NLMSFilter            nlms_;
    DoubleTalkDetector    dtd_;
    ResidualEchoEstimator ree_;
    AutomaticGainControl  agc_;
    VoiceActivityDetector vad_;
    MetricsCollector      metrics_;
    NeuralBridge          neural_;
    FrameCallback         callback_;

    // OLA state
    std::vector<float>    ola_accum_;   ///< Overlap-add accumulation buffer

    // Spectral working buffers (pre-allocated to avoid per-frame heap alloc)
    std::vector<float>    far_mag_;
    std::vector<float>    mic_mag_;
    std::vector<float>    echo_mag_;
    std::vector<float>    residual_mag_;
    std::vector<float>    mask_buf_;
};

} // namespace neural_echo::dsp
