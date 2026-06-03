#include <gtest/gtest.h>
#include <cmath>
#include <random>
#include <vector>
#include "nlms/nlms_filter.h"

using namespace neural_echo::dsp;

static std::vector<float> make_sine(float freq, float sr, std::size_t n) {
    std::vector<float> s(n);
    for (std::size_t i = 0; i < n; ++i)
        s[i] = std::sin(2.f * static_cast<float>(M_PI) * freq / sr * i);
    return s;
}

// ── Convergence: ERLE should increase over many frames ────────────────────────
TEST(NLMSFilterTest, ConvergesOnSyntheticEcho) {
    constexpr std::size_t SR     = 16000;
    constexpr std::size_t FRAME  = 160;
    constexpr std::size_t FRAMES = 200;
    constexpr float       FREQ   = 440.f;

    NLMSConfig cfg;
    cfg.filter_length = 512;
    cfg.step_size     = 0.5f;
    NLMSFilter filter(cfg);

    auto far_signal = make_sine(FREQ, SR, FRAMES * FRAME);

    // Simulate a simple room echo: 20-sample delay
    constexpr std::size_t DELAY = 20;
    std::vector<float> mic_signal(FRAMES * FRAME, 0.f);
    for (std::size_t i = DELAY; i < FRAMES * FRAME; ++i)
        mic_signal[i] = 0.8f * far_signal[i - DELAY];

    float first_erle = 0.f, last_erle = 0.f;

    for (std::size_t f = 0; f < FRAMES; ++f) {
        const float* far = far_signal.data() + f * FRAME;
        const float* mic = mic_signal.data() + f * FRAME;
        AudioFrame far_frame{far, FRAME, 0};
        AudioFrame mic_frame{mic, FRAME, 0};

        auto est = filter.process_frame(far_frame, mic_frame);

        // Compute ERLE for this frame
        float mic_pwr = 0.f, out_pwr = 0.f;
        for (std::size_t i = 0; i < FRAME; ++i) {
            mic_pwr += mic[i]  * mic[i];
            out_pwr += est.time_domain.samples[i] * est.time_domain.samples[i];
        }
        float erle = (out_pwr > 1e-9f)
                     ? 10.f * std::log10(mic_pwr / out_pwr)
                     : 0.f;

        if (f == 0)  first_erle = erle;
        if (f == FRAMES - 1) last_erle = erle;
    }

    // Filter should improve ERLE by at least 5 dB over 200 frames
    EXPECT_GT(last_erle, first_erle + 5.f)
        << "Expected convergence. First ERLE=" << first_erle
        << " dB, Last ERLE=" << last_erle << " dB";
}

// ── Freeze inhibits weight update ─────────────────────────────────────────────
TEST(NLMSFilterTest, FreezeInhibitsUpdate) {
    NLMSConfig cfg;
    cfg.filter_length = 64;
    NLMSFilter filter(cfg);

    std::vector<float> far(160, 0.5f);
    std::vector<float> mic(160, 0.3f);
    AudioFrame far_f{far.data(), 160, 0};
    AudioFrame mic_f{mic.data(), 160, 0};

    filter.process_frame(far_f, mic_f);

    filter.freeze();
    EXPECT_TRUE(filter.frozen());
    // Repeated processing with frozen filter should give identical outputs
    auto out1 = filter.process_frame(far_f, mic_f);
    auto out2 = filter.process_frame(far_f, mic_f);

    for (std::size_t i = 0; i < 160; ++i)
        EXPECT_FLOAT_EQ(out1.time_domain.samples[i], out2.time_domain.samples[i]);
}

// ── Reset clears weights ───────────────────────────────────────────────────────
TEST(NLMSFilterTest, ResetClearsState) {
    NLMSConfig cfg;
    cfg.filter_length = 64;
    NLMSFilter filter(cfg);

    std::vector<float> far(160, 1.f), mic(160, 1.f);
    AudioFrame far_f{far.data(), 160, 0};
    AudioFrame mic_f{mic.data(), 160, 0};
    for (int i = 0; i < 50; ++i) filter.process_frame(far_f, mic_f);

    filter.reset();
    EXPECT_FALSE(filter.frozen());
    // After reset, filter should start from zero again (higher residual)
    auto out = filter.process_frame(far_f, mic_f);
    EXPECT_GT(std::abs(out.time_domain.samples[0]), 1e-6f);
}
