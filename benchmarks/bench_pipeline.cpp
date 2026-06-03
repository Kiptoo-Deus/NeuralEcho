#include <benchmark/benchmark.h>
#include <vector>
#include <cmath>
#include <chrono>
#include "aec_pipeline.h"

using namespace neural_echo::dsp;

static std::vector<float> make_sine(float freq, float sr, std::size_t n) {
    std::vector<float> s(n);
    for (std::size_t i = 0; i < n; ++i)
        s[i] = std::sin(2.f * 3.14159f * freq / sr * static_cast<float>(i));
    return s;
}

// ── Full DSP pipeline latency (DSP-only, no neural) ──────────────────────────
static void BM_PipelineDSPOnly(benchmark::State& state) {
    AECConfig cfg;
    cfg.enable_neural = false;
    AECPipeline pipeline(cfg);

    auto far_s = make_sine(440.f, 16000.f, cfg.frame_size);
    auto mic_s = make_sine(440.f, 16000.f, cfg.frame_size, 0.8f);
    AudioFrame far_f{far_s.data(), cfg.frame_size, 0};
    AudioFrame mic_f{mic_s.data(), cfg.frame_size, 0};

    for (auto _ : state)
        benchmark::DoNotOptimize(pipeline.process(far_f, mic_f));

    state.SetItemsProcessed(state.iterations());
    // Report processing time vs real-time budget
    // At 16 kHz / 160 samples → 10 ms budget per frame
    state.SetLabel("frame_ms=10.0 budget");
}

BENCHMARK(BM_PipelineDSPOnly)->Iterations(10000);

// ── Latency histogram: measures P50 / P95 / P99 ──────────────────────────────
static void BM_PipelineLatencyHistogram(benchmark::State& state) {
    AECConfig cfg;
    cfg.enable_neural = false;
    AECPipeline pipeline(cfg);

    auto far_s = make_sine(440.f, 16000.f, cfg.frame_size);
    auto mic_s = make_sine(300.f, 16000.f, cfg.frame_size, 0.6f);
    AudioFrame far_f{far_s.data(), cfg.frame_size, 0};
    AudioFrame mic_f{mic_s.data(), cfg.frame_size, 0};

    std::vector<double> latencies_us;
    latencies_us.reserve(10000);

    for (auto _ : state) {
        auto t0 = std::chrono::steady_clock::now();
        benchmark::DoNotOptimize(pipeline.process(far_f, mic_f));
        auto t1 = std::chrono::steady_clock::now();
        latencies_us.push_back(
            std::chrono::duration<double, std::micro>(t1 - t0).count());
    }

    // Sort for percentile computation
    std::sort(latencies_us.begin(), latencies_us.end());
    const std::size_t N = latencies_us.size();
    if (N > 0) {
        state.counters["p50_us"] = latencies_us[N * 50 / 100];
        state.counters["p95_us"] = latencies_us[N * 95 / 100];
        state.counters["p99_us"] = latencies_us[std::min(N - 1, N * 99 / 100)];
        state.counters["p99_ms"] = latencies_us[std::min(N - 1, N * 99 / 100)] / 1000.0;
    }
}

BENCHMARK(BM_PipelineLatencyHistogram)->Iterations(10000);

// ── Multi-frame throughput (simulates real-time stream) ───────────────────────
static void BM_PipelineThroughput(benchmark::State& state) {
    const int batch = static_cast<int>(state.range(0));

    AECConfig cfg;
    cfg.enable_neural = false;
    AECPipeline pipeline(cfg);

    auto far_s = make_sine(440.f, 16000.f, cfg.frame_size);
    auto mic_s = make_sine(440.f, 16000.f, cfg.frame_size, 0.8f);
    AudioFrame far_f{far_s.data(), cfg.frame_size, 0};
    AudioFrame mic_f{mic_s.data(), cfg.frame_size, 0};

    for (auto _ : state) {
        for (int i = 0; i < batch; ++i)
            benchmark::DoNotOptimize(pipeline.process(far_f, mic_f));
    }

    // Total samples processed
    state.SetItemsProcessed(
        state.iterations() * static_cast<int64_t>(batch) *
        static_cast<int64_t>(cfg.frame_size));
    state.SetBytesProcessed(
        state.iterations() * static_cast<int64_t>(batch) *
        static_cast<int64_t>(cfg.frame_size) * sizeof(float));
}

BENCHMARK(BM_PipelineThroughput)->Arg(1)->Arg(10)->Arg(100)->Arg(1000);

// ── NLMS filter tap-count impact on pipeline ──────────────────────────────────
static void BM_PipelineVsFilterLength(benchmark::State& state) {
    const std::size_t taps = static_cast<std::size_t>(state.range(0));

    AECConfig cfg;
    cfg.nlms.filter_length = taps;
    cfg.enable_neural = false;
    AECPipeline pipeline(cfg);

    auto far_s = make_sine(440.f, 16000.f, cfg.frame_size);
    auto mic_s = make_sine(440.f, 16000.f, cfg.frame_size, 0.8f);
    AudioFrame far_f{far_s.data(), cfg.frame_size, 0};
    AudioFrame mic_f{mic_s.data(), cfg.frame_size, 0};

    for (auto _ : state)
        benchmark::DoNotOptimize(pipeline.process(far_f, mic_f));

    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(cfg.frame_size));
    state.SetLabel("taps=" + std::to_string(taps));
}

BENCHMARK(BM_PipelineVsFilterLength)
    ->Arg(128)->Arg(256)->Arg(512)->Arg(1024)->Arg(2048);

BENCHMARK_MAIN();
