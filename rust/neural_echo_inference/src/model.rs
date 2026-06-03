use anyhow::{Context, Result};
use ndarray::Array3;
use ort::{Session, SessionBuilder, Value};
use std::path::Path;
use tracing::info;

use neural_echo_buffers::{FeatureFrame, MaskFrame};

/// Wraps an ONNX Runtime session for NeuralEchoNet.
pub struct NeuralEchoModel {
    session:   Session,
    freq_bins: usize,
}

impl NeuralEchoModel {
    /// Load model from `onnx_path`. `num_threads` controls intra-op parallelism.
    pub fn load(onnx_path: impl AsRef<Path>, num_threads: usize, freq_bins: usize) -> Result<Self> {
        info!(path = ?onnx_path.as_ref(), "Loading ONNX model");

        let session = SessionBuilder::new()
            .context("Failed to create ONNX SessionBuilder")?
            .with_intra_threads(num_threads as i16)
            .context("Failed to set intra-op threads")?
            .commit_from_file(onnx_path)
            .context("Failed to load ONNX model from file")?;

        Ok(Self { session, freq_bins })
    }

    /// Run a single-frame forward pass.
    ///
    /// Input:  FeatureFrame  (3 × freq_bins)
    /// Output: MaskFrame     (freq_bins)
    pub fn run(&self, frame: &FeatureFrame) -> Result<MaskFrame> {
        let bins = self.freq_bins;

        // Build input tensor [1, 3, freq_bins]
        let mut input_data = vec![0.0f32; 3 * bins];
        input_data[..bins].copy_from_slice(&frame.far_spectrum);
        input_data[bins..2*bins].copy_from_slice(&frame.mic_spectrum);
        input_data[2*bins..].copy_from_slice(&frame.residual_spectrum);

        let input_array = Array3::from_shape_vec((1, 3, bins), input_data)
            .context("Failed to reshape input tensor")?;

        let input_value = Value::from_array(input_array)
            .context("Failed to create ONNX Value from array")?;

        // Run inference
        let outputs = self.session.run(ort::inputs!["input" => input_value]?)
            .context("ONNX session run failed")?;

        // Extract output mask [1, 1, freq_bins]
        let output_tensor = outputs["output"]
            .try_extract_tensor::<f32>()
            .context("Failed to extract output tensor")?;

        let mask_data: Vec<f32> = output_tensor.view()
            .iter()
            .cloned()
            .collect();

        Ok(MaskFrame {
            mask: mask_data,
            timestamp_ns: frame.timestamp_ns,
        })
    }
}
