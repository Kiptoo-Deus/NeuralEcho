#include <gtest/gtest.h>
#include <cmath>
#include <vector>
#include "aec_pipeline.h"

using namespace neural_echo::dsp;

// ── Helper: generate sine wave ────────────────────────────────────────────────
static std::vector<float> sine(float freq, float sr, std::size_t n,
                                float amp = 1.f) {
    std::vector<float> s(n);
    for (std::size_t i = 0; i < n; ++i)
        s[i] = amp * std::sin(2.f * static_cast<float>(M_PI) * freq / sr * i);
    return s;
}

// ── Pipeline constructs with default config ───────────────────────────────────
TEST(AECPipelineTest, ConstructsWithDefaults) {
    EXPECT_NO_THROW(AECPipeline pipeline);
}

// ── Output frame length matches input ─────────────────────────────────────────
TEST(AECPipelineTest, OutputLengthMatchesInput) {
    AECConfig cfg;
    AECPipeline pipeline(cfg);

    auto far_s = sine(440.f, 16000.f, cfg.frame_size);
    auto mic_s = sine(440.f, 16000.f, cfg.frame_size, 0.8f);

    AudioFrame far_f{far_s.data(), cfg.frame_size, 0};
    AudioFrame mic_f{mic_s.data(), cfg.frame_size, 0};

    auto out = pipeline.process(far_f, mic_f);
    EXPECT_EQ(out.samples.size(), cfg.frame_size);
}

// ── Frame-length mismatch throws ─────────────────────────────────────────────
TEST(AECPipelineTest, ThrowsOnFrameLengthMismatch) {
    AECPipeline pipeline;
    std::vector<float> far(80, 0.f);   // wrong size
    std::vector<float> mic(160, 0.f);

    AudioFrame far_f{far.data(), 80,  0};
    AudioFrame mic_f{mic.data(), 160, 0};

    EXPECT_THROW(pipeline.process(far_f, mic_f), std::invalid_argument);
}

// ── ERLE regression: pipeline achieves > 10 dB ERLE on synthetic echo ────────
TEST(AECPipelineTest, ERLERegressionAbove10dB) {
    constexpr std::size_t SR     = 16000;
    constexpr std::size_t FRAME  = 160;
    constexpr std::size_t FRAMES = 300;   // 3 seconds of adaptation

    AECConfig cfg;
    cfg.enable_neural = false;   // DSP-only for this regression
    AECPipeline pipeline(cfg);

    // Simple echo: far-end delayed by 15 samples, 0.7 gain
    auto far_full = sine(440.f, static_cast<float>(SR), FRAMES * FRAME);
    std::vector<float> mic_full(FRAMES * FRAME, 0.f);
    constexpr std::size_t DELAY = 15;
    for (std::size_t i = DELAY; i < FRAMES * FRAME; ++i)
        mic_full[i] = 0.7f * far_full[i - DELAY];

    float erle_sum = 0.f;
    int   erle_cnt = 0;

    for (std::size_t f = 0; f < FRAMES; ++f) {
        AudioFrame far_f{far_full.data() + f * FRAME, FRAME,
                         f * FRAME * (1000000000ULL / SR)};
        AudioFrame mic_f{mic_full.data() + f * FRAME, FRAME,
                         f * FRAME * (1000000000ULL / SR)};

        auto out = pipeline.process(far_f, mic_f);

        // Accumulate ERLE in second half (after convergence)
        if (f >= FRAMES / 2) {
            auto fm = pipeline.metrics().latest();
            erle_sum += fm.erle_db;
            ++erle_cnt;
        }
    }

    float mean_erle = erle_sum / static_cast<float>(erle_cnt);
    EXPECT_GT(mean_erle, 10.f)
        << "Expected > 10 dB mean ERLE after convergence, got " << mean_erle << " dB";
}

// ── Callback is invoked on every frame ────────────────────────────────────────
TEST(AECPipelineTest, CallbackInvokedEveryFrame) {
    AECPipeline pipeline;
    int count = 0;
    pipeline.set_frame_callback([&](const AudioBuffer&, const FrameMetrics&) {
        ++count;
    });

    auto far_s = sine(440.f, 16000.f, 160);
    auto mic_s = sine(440.f, 16000.f, 160, 0.8f);
    AudioFrame far_f{far_s.data(), 160, 0};
    AudioFrame mic_f{mic_s.data(), 160, 0};

    constexpr int N = 10;
    for (int i = 0; i < N; ++i)
        pipeline.process(far_f, mic_f);

    EXPECT_EQ(count, N);
}

// ── Reset clears metrics ──────────────────────────────────────────────────────
TEST(AECPipelineTest, ResetClearsMetrics) {
    AECPipeline pipeline;
    auto far_s = sine(440.f, 16000.f, 160);
    auto mic_s = sine(440.f, 16000.f, 160, 0.8f);
    AudioFrame far_f{far_s.data(), 160, 0};
    AudioFrame mic_f{mic_s.data(), 160, 0};

    for (int i = 0; i < 5; ++i) pipeline.process(far_f, mic_f);
    EXPECT_GT(pipeline.metrics().session().frames_processed, 0ULL);

    pipeline.reset();
    EXPECT_EQ(pipeline.metrics().session().frames_processed, 0ULL);
}
