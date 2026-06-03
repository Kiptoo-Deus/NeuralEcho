"""
Integration smoke test for the NeuralEchoNet ONNX model.

Usage (after training and exporting the model):
    pytest tests/integration/test_model_inference.py -v

Requires:
    pip install onnxruntime numpy pytest
"""

import numpy as np
import pytest

FREQ_BINS = 257


@pytest.fixture(scope="module")
def ort_session():
    """Load the quantised ONNX model once per test session."""
    ort = pytest.importorskip("onnxruntime")
    import pathlib
    model_path = pathlib.Path(__file__).parents[2] / "models" / "neural_echo_net_int8.onnx"
    if not model_path.exists():
        pytest.skip(f"Model not found: {model_path}. Run export_onnx.py first.")
    return ort.InferenceSession(str(model_path),
                                providers=["CPUExecutionProvider"])


def make_input(seed: int = 42) -> np.ndarray:
    """Random [1, 3, 257] float32 feature tensor."""
    rng = np.random.default_rng(seed)
    x = rng.random((1, 3, FREQ_BINS), dtype=np.float32)
    # Normalise each channel
    for c in range(3):
        x[0, c] /= (x[0, c].max() + 1e-8)
    return x


class TestModelInference:

    def test_output_shape(self, ort_session):
        """Model output must be [1, 1, 257]."""
        x = make_input()
        outputs = ort_session.run(None, {"input": x})
        assert len(outputs) == 1
        assert outputs[0].shape == (1, 1, FREQ_BINS), \
            f"Expected (1, 1, {FREQ_BINS}), got {outputs[0].shape}"

    def test_mask_range(self, ort_session):
        """All mask values must be in [0, 1]."""
        for seed in range(5):
            x    = make_input(seed)
            mask = ort_session.run(None, {"input": x})[0]
            assert mask.min() >= 0.0, f"Mask min {mask.min()} < 0"
            assert mask.max() <= 1.0, f"Mask max {mask.max()} > 1"

    def test_deterministic(self, ort_session):
        """Same input must produce identical output (no dropout at inference)."""
        x     = make_input()
        mask1 = ort_session.run(None, {"input": x})[0]
        mask2 = ort_session.run(None, {"input": x})[0]
        np.testing.assert_array_equal(mask1, mask2)

    def test_latency_under_2ms(self, ort_session):
        """Single-frame inference must complete in < 2 ms."""
        import time
        x = make_input()
        # Warm-up
        for _ in range(10):
            ort_session.run(None, {"input": x})
        # Measure 100 iterations
        times = []
        for _ in range(100):
            t0 = time.perf_counter()
            ort_session.run(None, {"input": x})
            times.append((time.perf_counter() - t0) * 1000)
        p99 = sorted(times)[98]
        assert p99 < 2.0, f"P99 inference latency {p99:.2f} ms exceeds 2 ms target"

    def test_suppression_effect(self, ort_session):
        """When mic is pure echo (same as far), mask should suppress heavily."""
        far = np.random.default_rng(0).random((1, 1, FREQ_BINS), dtype=np.float32)
        # mic ≈ far  (strong echo, no near-end speech)
        x = np.concatenate([far, far, far * 0.9], axis=1)
        mask = ort_session.run(None, {"input": x})[0]
        mean_mask = float(mask.mean())
        # We expect meaningful suppression (mask < 0.7 on average)
        assert mean_mask < 0.8, \
            f"Expected suppression mask < 0.8 for pure echo, got mean={mean_mask:.3f}"
