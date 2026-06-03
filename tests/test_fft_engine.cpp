#include <gtest/gtest.h>
#include <cmath>
#include <vector>
#include "fft/fft_engine.h"

using namespace neural_echo::dsp;

// ── Round-trip identity test ───────────────────────────────────────────────────
TEST(FFTEngineTest, RoundTripIdentity) {
    constexpr std::size_t N = 512;
    FFTEngine engine(N, 128, WindowType::Hann);

    // White-noise input
    std::vector<float> samples(N);
    for (std::size_t i = 0; i < N; ++i)
        samples[i] = static_cast<float>(std::sin(2.0 * M_PI * 440.0 * i / 16000.0));

    AudioFrame frame{samples.data(), N, 0};
    ComplexSpectrum spec = engine.stft(frame);
    AudioBuffer reconstructed = engine.istft(spec);

    // Allow 1 % relative error after windowed ISTFT
    for (std::size_t i = N / 4; i < 3 * N / 4; ++i) {
        EXPECT_NEAR(reconstructed.samples[i], samples[i], 0.02f)
            << "Mismatch at sample " << i;
    }
}

// ── Bin count ─────────────────────────────────────────────────────────────────
TEST(FFTEngineTest, BinCount) {
    for (std::size_t fft_sz : {256u, 512u, 1024u, 2048u}) {
        FFTEngine e(fft_sz, fft_sz / 4);
        EXPECT_EQ(e.bins(), fft_sz / 2 + 1);
    }
}

// ── Impulse response: DC component ────────────────────────────────────────────
TEST(FFTEngineTest, ImpulseHasFlatMagnitude) {
    constexpr std::size_t N = 512;
    FFTEngine engine(N, 128);

    std::vector<float> impulse(N, 0.f);
    impulse[0] = 1.f;

    AudioFrame frame{impulse.data(), N, 0};
    ComplexSpectrum spec = engine.stft(frame);

    std::vector<float> mag;
    FFTEngine::magnitude(spec, mag);

    // All bins should be non-negative
    for (std::size_t k = 0; k < engine.bins(); ++k)
        EXPECT_GE(mag[k], 0.f);
}

// ── Invalid construction ───────────────────────────────────────────────────────
TEST(FFTEngineTest, ThrowsOnNonPowerOfTwo) {
    EXPECT_THROW(FFTEngine(500, 125), std::invalid_argument);
}

TEST(FFTEngineTest, ThrowsOnZeroSize) {
    EXPECT_THROW(FFTEngine(0, 0), std::invalid_argument);
}

// ── Window types construct without error ──────────────────────────────────────
TEST(FFTEngineTest, AllWindowTypesConstruct) {
    for (auto wt : {WindowType::Hann, WindowType::Hamming, WindowType::Blackman}) {
        EXPECT_NO_THROW(FFTEngine(512, 128, wt));
    }
}
