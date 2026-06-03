#include <gtest/gtest.h>
#include <vector>
#include <cmath>
#include "ree/residual_echo_estimator.h"

using namespace neural_echo::dsp;

// ── Construction ──────────────────────────────────────────────────────────────
TEST(REETest, ConstructsWithDefaults) {
    EXPECT_NO_THROW(ResidualEchoEstimator ree);
}

TEST(REETest, ThrowsOnZeroBins) {
    REEConfig cfg; cfg.freq_bins = 0;
    EXPECT_THROW(ResidualEchoEstimator ree(cfg), std::invalid_argument);
}

// ── Output size ───────────────────────────────────────────────────────────────
TEST(REETest, OutputSizeMatchesFreqBins) {
    REEConfig cfg; cfg.freq_bins = 257;
    ResidualEchoEstimator ree(cfg);

    std::vector<float> mic(257, 0.5f);
    std::vector<float> echo(257, 0.3f);
    std::vector<float> residual;

    ree.estimate(mic, echo, residual);
    EXPECT_EQ(residual.size(), 257u);
}

// ── Spectral floor: residual is always non-negative ──────────────────────────
TEST(REETest, ResidualIsNonNegative) {
    ResidualEchoEstimator ree;

    // Large echo: echo > mic
    std::vector<float> mic(257, 0.1f);
    std::vector<float> echo(257, 1.0f);
    std::vector<float> residual;

    for (int i = 0; i < 10; ++i)
        ree.estimate(mic, echo, residual);

    for (float v : residual)
        EXPECT_GE(v, 0.f) << "Residual must be non-negative";
}

// ── When echo is zero, residual ≈ mic ─────────────────────────────────────────
TEST(REETest, ZeroEchoResidualApproxMic) {
    REEConfig cfg;
    cfg.smoothing_alpha   = 0.0f;  // No smoothing for instant response
    cfg.over_subtraction  = 1.0f;
    cfg.spectral_floor    = 0.0f;
    ResidualEchoEstimator ree(cfg);

    std::vector<float> mic(257, 0.5f);
    std::vector<float> echo(257, 0.0f);
    std::vector<float> residual;

    ree.estimate(mic, echo, residual);

    for (std::size_t k = 0; k < 257; ++k)
        EXPECT_NEAR(residual[k], 0.5f, 0.01f)
            << "With zero echo, residual should equal mic magnitude";
}

// ── Reset clears state ────────────────────────────────────────────────────────
TEST(REETest, ResetClearsState) {
    ResidualEchoEstimator ree;
    std::vector<float> mic(257, 1.f), echo(257, 0.5f), residual;
    for (int i = 0; i < 20; ++i) ree.estimate(mic, echo, residual);

    ree.reset();
    // After reset, a single low-power frame should yield small residual
    std::fill(mic.begin(),  mic.end(),  0.001f);
    std::fill(echo.begin(), echo.end(), 0.0f);
    ree.estimate(mic, echo, residual);
    for (float v : residual)
        EXPECT_LT(v, 0.1f) << "After reset, residual should reflect fresh low-power input";
}
