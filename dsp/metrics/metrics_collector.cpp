#include "metrics_collector.h"
#include <algorithm>

namespace neural_echo::dsp {

void MetricsCollector::push(const FrameMetrics& m) {
    std::lock_guard lock(mu_);
    latest_ = m;
    erle_accum_ += m.erle_db;
    rtf_accum_  += m.rtf;
    session_.frames_processed++;
    session_.mean_erle_db = erle_accum_ / static_cast<float>(session_.frames_processed);
    session_.mean_rtf     = rtf_accum_  / static_cast<float>(session_.frames_processed);
    if (session_.frames_processed == 1) {
        session_.min_erle_db = session_.max_erle_db = m.erle_db;
    } else {
        session_.min_erle_db = std::min(session_.min_erle_db, m.erle_db);
        session_.max_erle_db = std::max(session_.max_erle_db, m.erle_db);
    }
}

FrameMetrics MetricsCollector::latest() const {
    std::lock_guard lock(mu_);
    return latest_;
}

SessionMetrics MetricsCollector::session() const {
    std::lock_guard lock(mu_);
    return session_;
}

void MetricsCollector::reset() {
    std::lock_guard lock(mu_);
    latest_   = {};
    session_  = {};
    erle_accum_ = rtf_accum_ = 0.f;
}

} // namespace neural_echo::dsp
