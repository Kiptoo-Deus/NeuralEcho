#include "fft_engine.h"
#include <fftw3.h>
#include <cmath>
#include <stdexcept>
#include <numbers>
#include <cassert>

namespace neural_echo::dsp {

// ── Construction / destruction ─────────────────────────────────────────────────

FFTEngine::FFTEngine(std::size_t fft_size, std::size_t hop_size, WindowType window)
    : fft_size_(fft_size), hop_size_(hop_size), window_type_(window)
{
    if (fft_size_ == 0 || (fft_size_ & (fft_size_ - 1)) != 0)
        throw std::invalid_argument("FFTEngine: fft_size must be a power of two");
    if (hop_size_ == 0 || hop_size_ > fft_size_)
        throw std::invalid_argument("FFTEngine: invalid hop_size");

    build_window();
    allocate_plans();
}

FFTEngine::~FFTEngine() {
    destroy_plans();
}

FFTEngine::FFTEngine(FFTEngine&& o) noexcept
    : fft_size_(o.fft_size_), hop_size_(o.hop_size_),
      window_type_(o.window_type_), window_coeffs_(std::move(o.window_coeffs_)),
      plan_fwd_(o.plan_fwd_), plan_inv_(o.plan_inv_),
      fft_in_(o.fft_in_), fft_out_(o.fft_out_)
{
    o.plan_fwd_ = o.plan_inv_ = nullptr;
    o.fft_in_   = o.fft_out_  = nullptr;
}

FFTEngine& FFTEngine::operator=(FFTEngine&& o) noexcept {
    if (this != &o) {
        destroy_plans();
        fft_size_      = o.fft_size_;
        hop_size_      = o.hop_size_;
        window_type_   = o.window_type_;
        window_coeffs_ = std::move(o.window_coeffs_);
        plan_fwd_      = o.plan_fwd_;
        plan_inv_      = o.plan_inv_;
        fft_in_        = o.fft_in_;
        fft_out_       = o.fft_out_;
        o.plan_fwd_ = o.plan_inv_ = nullptr;
        o.fft_in_   = o.fft_out_  = nullptr;
    }
    return *this;
}

// ── Private helpers ────────────────────────────────────────────────────────────

void FFTEngine::build_window() {
    window_coeffs_.resize(fft_size_);
    const double N = static_cast<double>(fft_size_);
    const double pi = std::numbers::pi;
    for (std::size_t n = 0; n < fft_size_; ++n) {
        double x = 2.0 * pi * n / (N - 1.0);
        switch (window_type_) {
            case WindowType::Hann:
                window_coeffs_[n] = static_cast<float>(0.5 * (1.0 - std::cos(x)));
                break;
            case WindowType::Hamming:
                window_coeffs_[n] = static_cast<float>(0.54 - 0.46 * std::cos(x));
                break;
            case WindowType::Blackman:
                window_coeffs_[n] = static_cast<float>(
                    0.42 - 0.5 * std::cos(x) + 0.08 * std::cos(2.0 * x));
                break;
        }
    }
}

void FFTEngine::allocate_plans() {
    fft_in_  = fftwf_alloc_real(fft_size_);
    fft_out_ = fftwf_alloc_real(fft_size_ + 2);
    if (!fft_in_ || !fft_out_)
        throw std::runtime_error("FFTEngine: FFTW memory allocation failed");

    plan_fwd_ = fftwf_plan_dft_r2c_1d(
        static_cast<int>(fft_size_),
        fft_in_,
        reinterpret_cast<fftwf_complex*>(fft_out_),
        FFTW_MEASURE);

    plan_inv_ = fftwf_plan_dft_c2r_1d(
        static_cast<int>(fft_size_),
        reinterpret_cast<fftwf_complex*>(fft_out_),
        fft_in_,
        FFTW_MEASURE);

    if (!plan_fwd_ || !plan_inv_)
        throw std::runtime_error("FFTEngine: FFTW plan creation failed");
}

void FFTEngine::destroy_plans() {
    if (plan_fwd_) { fftwf_destroy_plan(plan_fwd_); plan_fwd_ = nullptr; }
    if (plan_inv_) { fftwf_destroy_plan(plan_inv_); plan_inv_ = nullptr; }
    if (fft_in_)   { fftwf_free(fft_in_);   fft_in_  = nullptr; }
    if (fft_out_)  { fftwf_free(fft_out_);  fft_out_ = nullptr; }
}

// ── Public API ─────────────────────────────────────────────────────────────────

ComplexSpectrum FFTEngine::stft(const AudioFrame& frame) const {
    assert(frame.length == fft_size_);

    // Apply window
    for (std::size_t i = 0; i < fft_size_; ++i)
        fft_in_[i] = frame.data[i] * window_coeffs_[i];

    fftwf_execute(plan_fwd_);

    const std::size_t n_bins = bins();
    ComplexSpectrum out;
    out.real.resize(n_bins);
    out.imag.resize(n_bins);
    out.timestamp_ns = frame.timestamp_ns;

    auto* cplx = reinterpret_cast<fftwf_complex*>(fft_out_);
    for (std::size_t k = 0; k < n_bins; ++k) {
        out.real[k] = cplx[k][0];
        out.imag[k] = cplx[k][1];
    }
    return out;
}

AudioBuffer FFTEngine::istft(const ComplexSpectrum& spectrum) const {
    assert(spectrum.bins() == bins());

    auto* cplx = reinterpret_cast<fftwf_complex*>(fft_out_);
    for (std::size_t k = 0; k < bins(); ++k) {
        cplx[k][0] = spectrum.real[k];
        cplx[k][1] = spectrum.imag[k];
    }

    fftwf_execute(plan_inv_);

    AudioBuffer out;
    out.samples.resize(fft_size_);
    out.timestamp_ns = spectrum.timestamp_ns;

    const float scale = 1.f / static_cast<float>(fft_size_);
    for (std::size_t i = 0; i < fft_size_; ++i)
        out.samples[i] = fft_in_[i] * scale;

    return out;
}

void FFTEngine::magnitude(const ComplexSpectrum& in, std::vector<float>& out) {
    out.resize(in.bins());
    for (std::size_t k = 0; k < in.bins(); ++k)
        out[k] = std::sqrt(in.real[k] * in.real[k] + in.imag[k] * in.imag[k]);
}

void FFTEngine::power(const ComplexSpectrum& in, std::vector<float>& out) {
    out.resize(in.bins());
    for (std::size_t k = 0; k < in.bins(); ++k)
        out[k] = in.real[k] * in.real[k] + in.imag[k] * in.imag[k];
}

} // namespace neural_echo::dsp
