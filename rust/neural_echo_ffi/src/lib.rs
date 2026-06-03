//! C-compatible FFI layer for the NeuralEcho neural runtime.
//!
//! This module exposes `extern "C"` functions that the C++ DSP engine
//! calls directly. cbindgen generates `neural_echo_ffi.h` from these
//! definitions via the `build.rs` script.
//!
//! # Safety
//! All pointer arguments MUST be non-null and point to valid memory
//! of the documented size. Callers are responsible for ensuring that
//! `NeuralContext` is not concurrently accessed from multiple threads.

#![allow(clippy::missing_safety_doc)]

use neural_echo_buffers::{FeatureFrame, MaskFrame, SpscRing};
use neural_echo_config::NeuralEchoConfig;
use neural_echo_inference::NeuralEchoModel;

// ── Public C types ─────────────────────────────────────────────────────────────

#[repr(C)]
pub enum NeuralStatus {
    Ok              = 0,
    QueueFull       = 1,
    MaskNotReady    = 2,
    InitFailed      = 3,
    NullPointer     = 4,
}

/// Opaque context handle passed to all FFI calls.
pub struct NeuralContext {
    model:        NeuralEchoModel,
    input_queue:  SpscRing<FeatureFrame>,
    output_queue: SpscRing<MaskFrame>,
    freq_bins:    usize,
    last_mask:    Vec<f32>,
}

// ── Lifecycle ──────────────────────────────────────────────────────────────────

/// Create a new NeuralContext.
///
/// # Parameters
/// * `config_path` — null-terminated UTF-8 path to `neural_echo.toml`.
///                   Pass null to use default configuration.
///
/// # Returns
/// Heap-allocated context pointer, or null on failure.
#[no_mangle]
pub unsafe extern "C" fn ne_context_create(
    config_path: *const std::ffi::c_char,
) -> *mut NeuralContext {
    let cfg = if config_path.is_null() {
        NeuralEchoConfig::default()
    } else {
        let path = std::ffi::CStr::from_ptr(config_path)
            .to_str()
            .unwrap_or("neural_echo.toml");
        neural_echo_config::load_or_default(path)
    };

    let freq_bins = cfg.model.freq_bins;

    let model = match NeuralEchoModel::load(
        &cfg.model.onnx_path,
        cfg.inference.num_threads,
        freq_bins,
    ) {
        Ok(m) => m,
        Err(_) => return std::ptr::null_mut(),
    };

    let cap = cfg.inference.queue_capacity;
    let ctx = Box::new(NeuralContext {
        model,
        input_queue:  SpscRing::new(cap),
        output_queue: SpscRing::new(cap),
        freq_bins,
        last_mask:    vec![1.0f32; freq_bins],
    });

    Box::into_raw(ctx)
}

/// Destroy a NeuralContext created by `ne_context_create`.
#[no_mangle]
pub unsafe extern "C" fn ne_context_destroy(ctx: *mut NeuralContext) {
    if !ctx.is_null() {
        drop(Box::from_raw(ctx));
    }
}

// ── Per-frame processing ───────────────────────────────────────────────────────

/// Process one spectral frame.
///
/// # Parameters
/// * `far_spectrum`      — pointer to `freq_bins` f32 values (far-end magnitude)
/// * `mic_spectrum`      — pointer to `freq_bins` f32 values (mic magnitude)
/// * `residual_spectrum` — pointer to `freq_bins` f32 values (residual echo PSD)
/// * `mask_out`          — output buffer, must hold `freq_bins` f32 values
///
/// # Returns
/// * `NeuralStatus::Ok`          — mask written to `mask_out`
/// * `NeuralStatus::MaskNotReady`— previous mask written (inference still running)
/// * `NeuralStatus::QueueFull`   — input dropped; last mask returned
#[no_mangle]
pub unsafe extern "C" fn ne_process_frame(
    ctx:               *mut NeuralContext,
    far_spectrum:      *const f32,
    mic_spectrum:      *const f32,
    residual_spectrum: *const f32,
    mask_out:          *mut f32,
) -> NeuralStatus {
    if ctx.is_null() || far_spectrum.is_null()
        || mic_spectrum.is_null() || residual_spectrum.is_null()
        || mask_out.is_null()
    {
        return NeuralStatus::NullPointer;
    }

    let ctx = &mut *ctx;
    let bins = ctx.freq_bins;

    // Build feature frame from C pointers
    let frame = FeatureFrame {
        far_spectrum:      std::slice::from_raw_parts(far_spectrum, bins).to_vec(),
        mic_spectrum:      std::slice::from_raw_parts(mic_spectrum, bins).to_vec(),
        residual_spectrum: std::slice::from_raw_parts(residual_spectrum, bins).to_vec(),
        timestamp_ns:      0,
    };

    // Run inference synchronously (for now; async offload added in ne_context_spawn_worker)
    let status = match ctx.model.run(&frame) {
        Ok(mask_frame) => {
            ctx.last_mask.copy_from_slice(&mask_frame.mask);
            NeuralStatus::Ok
        }
        Err(_) => NeuralStatus::MaskNotReady,
    };

    // Write mask to output
    std::ptr::copy_nonoverlapping(ctx.last_mask.as_ptr(), mask_out, bins);
    status
}
