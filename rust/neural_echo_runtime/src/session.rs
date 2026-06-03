use anyhow::Result;
use neural_echo_buffers::{FeatureFrame, MaskFrame, SpscRing};
use neural_echo_inference::NeuralEchoModel;
use tracing::{error, trace};

/// Runs the inference loop on a dedicated thread.
///
/// Consumes `FeatureFrame`s from `input_rx` and pushes `MaskFrame`s to `output_tx`.
/// The loop exits when `input_rx` is dropped (disconnected).
pub async fn inference_loop(
    model:     NeuralEchoModel,
    input_rx:  SpscRing<FeatureFrame>,
    output_tx: SpscRing<MaskFrame>,
) {
    loop {
        // Spin-wait for a frame — yields to the async executor each iteration.
        let frame = loop {
            if let Some(f) = input_rx.pop() {
                break f;
            }
            tokio::task::yield_now().await;
        };

        match model.run(&frame) {
            Ok(mask) => {
                trace!(timestamp = mask.timestamp_ns, "Mask computed");
                // If the output queue is full, drop the oldest mask (overwrite).
                if output_tx.push(mask).is_err() {
                    if let Some(stale) = output_tx.pop() {
                        let _ = output_tx.push(stale);
                    }
                }
            }
            Err(e) => {
                error!("Inference error: {e:?}");
            }
        }
    }
}
