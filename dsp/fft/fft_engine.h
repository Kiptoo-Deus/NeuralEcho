#pragma once
#include <cstddef>
#include <memory>
#include <vector>
#include "audio_frame.h"

// Forward-declare FFTW plan to avoid pulling in fftw3.h in every TU.
typedef struct fftwf_plan_s* fftwf_plan;

namespace neural_echo::dsp {

enum class WindowType { Hann, Hamming, Blackman };

/// Wraps FFTW3 single-precision plans for STFT / ISTFT.
///
/// Thread-safety: not thread-safe. Each audio thread should own its own
/// FFTEngine instance.
class FFTEngine {
public:
    /// @param fft_size   Must be a power of two in [256, 4096].
    /// @param hop_size   Typically fft_size / 4 (75 % overlap).
    /// @param window     Window function applied before each FFT frame.
    explicit FFTEngine(std::size_t fft_size  = 512,
                       std::size_t hop_size  = 128,
                       WindowType  window    = WindowType::Hann);
    ~FFTEngine();

    // Non-copyable, movable.
    FFTEngine(const FFTEngine&)            = delete;
    FFTEngine& operator=(const FFTEngine&) = delete;
    FFTEngine(FFTEngine&&)                 noexcept;
    FFTEngine& operator=(FFTEngine&&)      noexcept;

    // ── Core transforms ────────────────────────────────────────────────────────

    /// Compute a single windowed FFT frame.
    /// @param frame  Must have exactly fft_size() samples.
    ComplexSpectrum stft(const AudioFrame& frame) const;

    /// Inverse FFT — returns a time-domain frame of fft_size() samples.
    /// Caller is responsible for overlap-add reconstruction.
    AudioBuffer istft(const ComplexSpectrum& spectrum) const;

    // ── Utility ────────────────────────────────────────────────────────────────

    std::size_t fft_size() const noexcept  { return fft_size_; }
    std::size_t hop_size() const noexcept  { return hop_size_; }
    std::size_t bins()     const noexcept  { return fft_size_ / 2 + 1; }

    /// In-place magnitude spectrum (|X[k]|).
    static void magnitude(const ComplexSpectrum& in, std::vector<float>& out);

    /// In-place power spectrum (|X[k]|^2).
    static void power(const ComplexSpectrum& in, std::vector<float>& out);

private:
    void build_window();
    void allocate_plans();
    void destroy_plans();

    std::size_t  fft_size_;
    std::size_t  hop_size_;
    WindowType   window_type_;
    std::vector<float> window_coeffs_;

    // FFTW plans (raw pointers managed manually to avoid fftw3.h in header).
    fftwf_plan plan_fwd_  = nullptr;
    fftwf_plan plan_inv_  = nullptr;

    // FFTW-aligned working buffers.
    float* fft_in_  = nullptr;   ///< size = fft_size_
    float* fft_out_ = nullptr;   ///< size = fft_size_ + 2 (real interleaved)
};

} // namespace neural_echo::dsp
