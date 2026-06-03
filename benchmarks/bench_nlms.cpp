#include <benchmark/benchmark.h>
#include <vector>
#include <cmath>
#include "nlms/nlms_filter.h"

using namespace neural_echo::dsp;

static void BM_NLMSFilter(benchmark::State& state) {
    const std::size_t taps  = static_cast<std::size_t>(state.range(0));
    const std::size_t frame = 160;

    NLMSConfig cfg;
    cfg.filter_length = taps;
    cfg.enable_simd   = (state.range(1) == 1);
    NLMSFilter filter(cfg);

    std::vector<float> far(frame), mic(frame);
    for (std::size_t i = 0; i < frame; ++i) {
        far[i] = std::sin(2.f * 3.14159f * 440.f / 16000.f * i);
        mic[i] = 0.8f * far[i];
    }

    AudioFrame far_f{far.data(), frame, 0};
    AudioFrame mic_f{mic.data(), frame, 0};

    for (auto _ : state)
        benchmark::DoNotOptimize(filter.process_frame(far_f, mic_f));

    // Report throughput in samples/second
    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(frame));
    state.SetLabel("taps=" + std::to_string(taps) +
                   (cfg.enable_simd ? " SIMD" : " scalar"));
}

// Args: {filter_length, simd_enabled}
BENCHMARK(BM_NLMSFilter)
    ->Args({128,  0})->Args({128,  1})
    ->Args({512,  0})->Args({512,  1})
    ->Args({1024, 0})->Args({1024, 1})
    ->Args({2048, 0})->Args({2048, 1});

// ── Filter with freeze (no weight update) ─────────────────────────────────────
static void BM_NLMSFilterFrozen(benchmark::State& state) {
    NLMSConfig cfg;
    cfg.filter_length = 512;
    NLMSFilter filter(cfg);
    filter.freeze();

    std::vector<float> far(160, 0.5f), mic(160, 0.4f);
    AudioFrame far_f{far.data(), 160, 0};
    AudioFrame mic_f{mic.data(), 160, 0};

    for (auto _ : state)
        benchmark::DoNotOptimize(filter.process_frame(far_f, mic_f));

    state.SetItemsProcessed(state.iterations() * 160);
    state.SetLabel("frozen=true");
}

BENCHMARK(BM_NLMSFilterFrozen);

BENCHMARK_MAIN();
