#include "neural_bridge.h"
#include <cstring>
#include <stdexcept>
#include <algorithm>

// Include the auto-generated cbindgen header.
// When building without neural support, stub functions are used.
#if __has_include("../rust/neural_echo_ffi/include/neural_echo_ffi.h")
#  include "../rust/neural_echo_ffi/include/neural_echo_ffi.h"
#  define NE_HAS_FFI 1
#else
// ── Stub declarations so the translation unit compiles without Rust ──────────
extern "C" {
    struct NeuralContext;
    enum NeuralStatus { Ok=0, QueueFull=1, MaskNotReady=2, InitFailed=3, NullPointer=4 };
    NeuralContext* ne_context_create(const char*) { return nullptr; }
    void           ne_context_destroy(NeuralContext*) {}
    NeuralStatus   ne_process_frame(NeuralContext*, const float*, const float*,
                                    const float*, float*) { return NeuralStatus::InitFailed; }
}
#  define NE_HAS_FFI 0
#endif

namespace neural_echo::dsp {

NeuralBridge::NeuralBridge(const std::string& config_path,
                            bool               enabled,
                            std::size_t        freq_bins)
    : enabled_(enabled), freq_bins_(freq_bins)
{
    unity_mask_.assign(freq_bins_, 1.f);

    if (!enabled_) return;

    const char* path = config_path.empty() ? nullptr : config_path.c_str();
    ctx_ = ne_context_create(path);
    // If ctx_ is null the model file was missing — we silently run without neural.
}

NeuralBridge::~NeuralBridge() {
    if (ctx_) {
        ne_context_destroy(ctx_);
        ctx_ = nullptr;
    }
}

NeuralBridge::NeuralBridge(NeuralBridge&& o) noexcept
    : ctx_(o.ctx_), enabled_(o.enabled_),
      freq_bins_(o.freq_bins_), unity_mask_(std::move(o.unity_mask_))
{
    o.ctx_ = nullptr;
}

NeuralBridge& NeuralBridge::operator=(NeuralBridge&& o) noexcept {
    if (this != &o) {
        if (ctx_) ne_context_destroy(ctx_);
        ctx_        = o.ctx_;
        enabled_    = o.enabled_;
        freq_bins_  = o.freq_bins_;
        unity_mask_ = std::move(o.unity_mask_);
        o.ctx_      = nullptr;
    }
    return *this;
}

NeuralBridgeStatus NeuralBridge::process_frame(const float* far_spectrum,
                                                const float* mic_spectrum,
                                                const float* residual_spectrum,
                                                float*       mask_out)
{
    if (!enabled_) {
        std::copy(unity_mask_.begin(), unity_mask_.end(), mask_out);
        return NeuralBridgeStatus::Disabled;
    }

    if (!ctx_) {
        // Model not loaded — fall back to unity mask
        std::copy(unity_mask_.begin(), unity_mask_.end(), mask_out);
        return NeuralBridgeStatus::InitFailed;
    }

    auto status = static_cast<NeuralBridgeStatus>(
        static_cast<int>(
            ne_process_frame(ctx_, far_spectrum, mic_spectrum,
                             residual_spectrum, mask_out)));
    return status;
}

void NeuralBridge::apply_mask(const float*   mask,
                               ComplexSpectrum& spectrum,
                               std::size_t    bins)
{
    for (std::size_t k = 0; k < bins && k < spectrum.real.size(); ++k) {
        spectrum.real[k] *= mask[k];
        spectrum.imag[k] *= mask[k];
    }
}

} // namespace neural_echo::dsp
