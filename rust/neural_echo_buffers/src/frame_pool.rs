use crate::{FeatureFrame, MaskFrame};
use crossbeam::queue::ArrayQueue;
use std::sync::Arc;

/// Pre-allocated pool of reusable FeatureFrames to avoid heap allocation on
/// the hot audio path.
pub struct FramePool {
    pool:      Arc<ArrayQueue<FeatureFrame>>,
    freq_bins: usize,
}

impl FramePool {
    pub fn new(capacity: usize, freq_bins: usize) -> Self {
        let pool = Arc::new(ArrayQueue::new(capacity));
        for _ in 0..capacity {
            let _ = pool.push(FeatureFrame {
                far_spectrum:      vec![0.0; freq_bins],
                mic_spectrum:      vec![0.0; freq_bins],
                residual_spectrum: vec![0.0; freq_bins],
                timestamp_ns:      0,
            });
        }
        Self { pool, freq_bins }
    }

    /// Acquire a frame from the pool, or allocate a fresh one if the pool is empty.
    pub fn acquire(&self) -> FeatureFrame {
        self.pool.pop().unwrap_or_else(|| FeatureFrame {
            far_spectrum:      vec![0.0; self.freq_bins],
            mic_spectrum:      vec![0.0; self.freq_bins],
            residual_spectrum: vec![0.0; self.freq_bins],
            timestamp_ns:      0,
        })
    }

    /// Return a frame to the pool. If the pool is full, the frame is dropped.
    pub fn release(&self, frame: FeatureFrame) {
        let _ = self.pool.push(frame);
    }
}
