#pragma once
#include <atomic>
#include <cstdint>
#include <mutex>

namespace neural_echo::dsp {

struct FrameMetrics {
    float    erle_db        = 0.f;   ///< Echo Return Loss Enhancement (dB)
    float    mic_power_db   = 0.f;
    float    out_power_db   = 0.f;
    float    rtf            = 0.f;   ///< Real-Time Factor (proc_time / frame_time)
    uint64_t timestamp_ns   = 0;
};

struct SessionMetrics {
    float    mean_erle_db   = 0.f;
    float    min_erle_db    = 0.f;
    float    max_erle_db    = 0.f;
    float    mean_rtf       = 0.f;
    uint64_t frames_processed = 0;
};

class MetricsCollector {
public:
    void push(const FrameMetrics& m);
    FrameMetrics   latest()  const;
    SessionMetrics session() const;
    void           reset();

private:
    mutable std::mutex mu_;
    FrameMetrics   latest_{};
    SessionMetrics session_{};
    float          erle_accum_ = 0.f;
    float          rtf_accum_  = 0.f;
};

} // namespace neural_echo::dsp
