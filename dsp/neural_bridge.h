#pragma once
#include <cstddef>
#include <vector>
#include <string>
#include <memory>
#include "audio_frame.h"

// Forward-declare the opaque Rust context type
struct NeuralContext;

namespace neural_echo::dsp {

/// Status codes returned by the Rust neural runtime.
enum class NeuralBridgeStatus {
    Ok           = 0,
    QueueFull    = 1,
    MaskNotReady = 2,
    InitFailed   = 3,
    NullPointer  = 4,
    Disabled     = 5,   ///< Neural processing disabled in config
};

/// C++ RAII wrapper around the Rust NeuralContext FFI.
///
/// Loads the ONNX model, manages context lifetime, and exposes a
/// single process_frame() call that returns a spectral suppression mask.
///
/// If the model file is missing or neural processing is disabled, the bridge
/// silently falls back to a unity mask (all-ones), so the DSP pipeline always
/// produces output.
class NeuralBridge {
public:
    /// @param config_path  Path to neural_echo.toml. Empty → use defaults.
    /// @param enabled      If false, bridge is a no-op unity-mask passthrough.
    explicit NeuralBridge(const std::string& config_path = "",
                          bool               enabled     = true,
                          std::size_t        freq_bins   = 257);

    ~NeuralBridge();

    // Non-copyable, movable
    NeuralBridge(const NeuralBridge&)            = delete;
    NeuralBridge& operator=(const NeuralBridge&) = delete;
    NeuralBridge(NeuralBridge&&)                 noexcept;
    NeuralBridge& operator=(NeuralBridge&&)      noexcept;

    /// Run neural inference for one spectral frame.
    ///
    /// Fills mask_out[0..freq_bins] with per-bin suppression weights in [0,1].
    /// On failure (model not loaded, queue full) fills with the last valid mask.
    ///
    /// @param far_spectrum       [freq_bins] far-end magnitude spectrum
    /// @param mic_spectrum       [freq_bins] microphone magnitude spectrum
    /// @param residual_spectrum  [freq_bins] residual echo magnitude spectrum
    /// @param mask_out           [freq_bins] output suppression mask
    NeuralBridgeStatus process_frame(const float* far_spectrum,
                                     const float* mic_spectrum,
                                     const float* residual_spectrum,
                                     float*       mask_out);

    bool is_loaded()  const noexcept { return ctx_ != nullptr; }
    bool is_enabled() const noexcept { return enabled_; }
    std::size_t freq_bins() const noexcept { return freq_bins_; }

    /// Apply mask to a complex spectrum in-place.
    /// enhanced[k] = mask[k] * spectrum[k]  (applied to both real & imag)
    static void apply_mask(const float*   mask,
                           ComplexSpectrum& spectrum,
                           std::size_t    bins);

private:
    NeuralContext* ctx_       = nullptr;
    bool           enabled_   = true;
    std::size_t    freq_bins_ = 257;
    std::vector<float> unity_mask_;   ///< Fallback all-ones mask
};

} // namespace neural_echo::dsp
