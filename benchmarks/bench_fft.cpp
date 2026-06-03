#include <benchmark/benchmark.h>
#include <vector>
#include <cmath>
#include "fft/fft_engine.h"

using namespace neural_echo::dsp;

// ── STFT throughput for various FFT sizes ─────────────────────────────────────
static void BM_STFT(benchmark::State& state) {
    const std::size_t fft_size = static_cast<std::size_t>(state.range(0));
    FFTEngine engine(fft_size, fft_size / 4);

    std::vector<float> samples(fft_size);
    for (std::size_t i = 0; i < fft_size; ++i)
        samples[i] = std::sin(2.f * 3.14159f * 440.f / 16000.f * i);

    AudioFrame frame{samples.data(), fft_size, 0};

    for (auto _ : state)
        benchmark::DoNotOptimize(engine.stft(frame));

    state.SetItemsProcessed(state.iterations());
    state.SetLabel("size=" + std::to_string(fft_size));
}

BENCHMARK(BM_STFT)->Arg(256)->Arg(512)->Arg(1024)->Arg(2048);

// ── ISTFT throughput ──────────────────────────────────────────────────────────
static void BM_ISTFT(benchmark::State& state) {
    const std::size_t fft_size = static_cast<std::size_t>(state.range(0));
    FFTEngine engine(fft_size, fft_size / 4);

    std::vector<float> samples(fft_size, 0.5f);
    AudioFrame frame{samples.data(), fft_size, 0};
    ComplexSpectrum spec = engine.stft(frame);

    for (auto _ : state)
        benchmark::DoNotOptimize(engine.istft(spec));

    state.SetItemsProcessed(state.iterations());
}

BENCHMARK(BM_ISTFT)->Arg(256)->Arg(512)->Arg(1024)->Arg(2048);

// ── Round-trip (STFT + ISTFT) ─────────────────────────────────────────────────
static void BM_RoundTrip(benchmark::State& state) {
    FFTEngine engine(512, 128);
    std::vector<float> samples(512, 0.1f);
    AudioFrame frame{samples.data(), 512, 0};

    for (auto _ : state) {
        auto spec = engine.stft(frame);
        benchmark::DoNotOptimize(engine.istft(spec));
    }
    state.SetItemsProcessed(state.iterations());
}

BENCHMARK(BM_RoundTrip);

BENCHMARK_MAIN();
