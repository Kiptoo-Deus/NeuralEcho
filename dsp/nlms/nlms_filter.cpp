#include "nlms_filter.h"
#include <cmath>
#include <stdexcept>
#include <numeric>
#include <cassert>

// Optionally pull in xsimd for SIMD dot product
#ifdef __has_include
#  if __has_include(<xsimd/xsimd.hpp>)
#    include <xsimd/xsimd.hpp>
#    define NE_HAVE_XSIMD 1
#  endif
#endif

namespace neural_echo::dsp {

// ── Helpers ────────────────────────────────────────────────────────────────────

#ifdef NE_HAVE_XSIMD
static float simd_dot(const float* __restrict a,
                       const float* __restrict b,
                       std::size_t n) noexcept
{
    using batch = xsimd::batch<float>;
    constexpr std::size_t simd_size = batch::size;
    float result = 0.f;

    std::size_t i = 0;
    if (n >= simd_size) {
        auto acc = xsimd::batch<float>(0.f);
        for (; i + simd_size <= n; i += simd_size)
            acc = xsimd::fma(xsimd::load_unaligned(a + i),
                             xsimd::load_unaligned(b + i), acc);
        result = xsimd::reduce_add(acc);
    }
    for (; i < n; ++i) result += a[i] * b[i];
    return result;
}
#else
static float simd_dot(const float* a, const float* b, std::size_t n) noexcept {
    float s = 0.f;
    for (std::size_t i = 0; i < n; ++i) s += a[i] * b[i];
    return s;
}
#endif

// ── NLMSFilter ─────────────────────────────────────────────────────────────────

NLMSFilter::NLMSFilter(NLMSConfig cfg) : cfg_(cfg) {
    if (cfg_.filter_length == 0)
        throw std::invalid_argument("NLMSFilter: filter_length must be > 0");
    weights_.assign(cfg_.filter_length, 0.f);
    delay_line_.assign(cfg_.filter_length, 0.f);
}

void NLMSFilter::reset() {
    std::fill(weights_.begin(), weights_.end(), 0.f);
    std::fill(delay_line_.begin(), delay_line_.end(), 0.f);
    delay_idx_ = 0;
    frozen_    = false;
}

float NLMSFilter::process_sample(float x, float d) {
    const std::size_t L = cfg_.filter_length;

    // Insert new far-end sample into circular delay line
    delay_line_[delay_idx_] = x;
    delay_idx_ = (delay_idx_ + 1) % L;

    // Build contiguous snapshot of delay line (newest first)
    // For simplicity, copy into a linear buffer.
    // A production implementation would use a double-buffer to avoid the copy.
    static thread_local std::vector<float> snapshot;
    snapshot.resize(L);
    for (std::size_t i = 0; i < L; ++i)
        snapshot[i] = delay_line_[(delay_idx_ + L - 1 - i) % L];

    // Estimate: y[n] = w^T * x
    float y = simd_dot(weights_.data(), snapshot.data(), L);
    float e = d - y;   // Error (residual)

    if (!frozen_) {
        // Power normalisation
        float power = simd_dot(snapshot.data(), snapshot.data(), L)
                      + cfg_.regularisation;
        float mu_n = cfg_.step_size / power;

        // Weight update: w += mu_n * e * x  (with leakage)
        for (std::size_t i = 0; i < L; ++i)
            weights_[i] = (1.f - cfg_.leakage) * weights_[i]
                          + mu_n * e * snapshot[i];
    }

    return e;
}

EchoEstimate NLMSFilter::process_frame(const AudioFrame& far_end,
                                        const AudioFrame& mic)
{
    assert(far_end.length == mic.length);
    const std::size_t N = far_end.length;

    output_buffer_.samples.resize(N);
    output_buffer_.timestamp_ns = mic.timestamp_ns;

    for (std::size_t n = 0; n < N; ++n)
        output_buffer_.samples[n] = process_sample(far_end.data[n], mic.data[n]);

    EchoEstimate est;
    est.time_domain = output_buffer_;
    return est;
}

} // namespace neural_echo::dsp
