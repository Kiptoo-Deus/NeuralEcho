use serde::{Deserialize, Serialize};
use std::path::PathBuf;

/// Top-level NeuralEcho runtime configuration.
/// Loaded from `neural_echo.toml` at startup.
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct NeuralEchoConfig {
    pub model:     ModelConfig,
    pub inference: InferenceConfig,
    pub audio:     AudioConfig,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct ModelConfig {
    /// Path to the ONNX model file.
    pub onnx_path: PathBuf,
    /// Use INT8 quantised weights if available.
    pub quantised:  bool,
    /// Number of frequency bins (fft_size/2 + 1).
    pub freq_bins:  usize,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct InferenceConfig {
    /// Number of inference worker threads.
    pub num_threads:      usize,
    /// SPSC ring buffer capacity in frames.
    pub queue_capacity:   usize,
    /// Fall back to last valid mask if queue is empty.
    pub mask_persistence: bool,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct AudioConfig {
    pub sample_rate: u32,
    pub frame_size:  usize,
    pub fft_size:    usize,
}

impl Default for NeuralEchoConfig {
    fn default() -> Self {
        Self {
            model: ModelConfig {
                onnx_path: PathBuf::from("models/neural_echo_net_int8.onnx"),
                quantised: true,
                freq_bins: 257,
            },
            inference: InferenceConfig {
                num_threads:      1,
                queue_capacity:   8,
                mask_persistence: true,
            },
            audio: AudioConfig {
                sample_rate: 16000,
                frame_size:  160,
                fft_size:    512,
            },
        }
    }
}
