#pragma once
#include <functional>
#include <memory>
#include "audio_frame.h"
#include "fft/fft_engine.h"
#include "nlms/nlms_filter.h"
#include "dtd/double_talk_detector.h"
#include "agc/agc.h"
#include "vad/vad.h"
#include "metrics/metrics_collector.h"

namespace neural_echo::dsp {

struct AECConfig {
    std::size_t sample_rate    = 16000;
    std::size_t frame_size     = 160;    ///< 10 ms @ 16 kHz
    std::size_t fft_size       = 512;
    std::size_t hop_size       = 128;
    NLMSConfig  nlms;
    DTDConfig   dtd;
    AGCConfig   agc;
    VADConfig   vad;
    bool        enable_neural  = true;
};

using FrameCallback = std::function<void(const AudioBuffer&, const FrameMetrics&)>;

/// Top-level AEC pipeline facade.
/// Owns all DSP modules and drives per-frame processing.
class AECPipeline {
public:
    explicit AECPipeline(AECConfig cfg = {});
    ~AECPipeline();

    AECPipeline(const AECPipeline&)            = delete;
    AECPipeline& operator=(const AECPipeline&) = delete;
    AECPipeline(AECPipeline&&)                 = default;

    /// Process one pair of frames synchronously.
    /// @param far_end   Loudspeaker reference signal (length == cfg.frame_size)
    /// @param mic       Microphone signal            (length == cfg.frame_size)
    /// @returns         Echo-cancelled output frame
    AudioBuffer process(const AudioFrame& far_end, const AudioFrame& mic);

    /// Register a callback invoked after every processed frame.
    void set_frame_callback(FrameCallback cb) { callback_ = std::move(cb); }

    const AECConfig&    config()  const { return cfg_; }
    const MetricsCollector& metrics() const { return metrics_; }

    void reset();

private:
    AECConfig         cfg_;
    FFTEngine         fft_;
    NLMSFilter        nlms_;
    DoubleTalkDetector dtd_;
    AutomaticGainControl agc_;
    VoiceActivityDetector vad_;
    MetricsCollector  metrics_;
    FrameCallback     callback_;

    // Overlap-add buffers
    AudioBuffer       ola_buffer_;
};

} // namespace neural_echo::dsp
