pub mod spsc_ring;
pub mod frame_pool;

pub use spsc_ring::SpscRing;
pub use frame_pool::FramePool;

/// A single spectral feature frame passed from C++ DSP to Rust inference.
#[derive(Clone, Debug)]
pub struct FeatureFrame {
    /// Far-end magnitude spectrum  [freq_bins]
    pub far_spectrum:      Vec<f32>,
    /// Mic magnitude spectrum      [freq_bins]
    pub mic_spectrum:      Vec<f32>,
    /// Residual echo estimate      [freq_bins]
    pub residual_spectrum: Vec<f32>,
    pub timestamp_ns:      u64,
}

/// Suppression mask returned from Rust to C++.
#[derive(Clone, Debug)]
pub struct MaskFrame {
    /// Per-bin suppression weights in [0, 1]
    pub mask:         Vec<f32>,
    pub timestamp_ns: u64,
}
