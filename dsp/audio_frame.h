#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>
#include <span>

namespace neural_echo::dsp {

/// Immutable view of a single mono PCM audio frame.
struct AudioFrame {
    const float* data      = nullptr;
    std::size_t  length    = 0;
    uint64_t     timestamp_ns = 0;   ///< Monotonic clock nanoseconds

    std::span<const float> samples() const { return {data, length}; }
};

/// Owning audio buffer — used for pipeline-internal storage.
struct AudioBuffer {
    std::vector<float> samples;
    uint64_t           timestamp_ns = 0;

    AudioFrame view() const {
        return { samples.data(), samples.size(), timestamp_ns };
    }

    void resize(std::size_t n, float fill = 0.f) {
        samples.assign(n, fill);
    }
};

/// Complex spectrum produced by FFTEngine::stft().
struct ComplexSpectrum {
    std::vector<float> real;   ///< size = fft_size/2 + 1
    std::vector<float> imag;
    uint64_t           timestamp_ns = 0;

    std::size_t bins() const { return real.size(); }

    void magnitude(std::vector<float>& out) const;
    void power    (std::vector<float>& out) const;
};

/// Echo estimate from the adaptive filter.
struct EchoEstimate {
    AudioBuffer  time_domain;
    ComplexSpectrum spectrum;
};

/// Suppression mask from the neural layer — values in [0, 1]^bins.
struct SuppressionMask {
    std::vector<float> values;   ///< size = fft_size/2 + 1
    bool               valid = false;
};

} // namespace neural_echo::dsp
