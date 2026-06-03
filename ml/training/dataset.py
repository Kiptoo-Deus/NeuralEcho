"""
AEC Challenge dataset loader
=============================
Expects the Microsoft AEC Challenge directory layout:

  data_dir/
    train/
      farend_speech/   *.wav
      nearend_mic/     *.wav
      nearend_speech/  *.wav   (clean reference)
    val/
      ...

Each sample returns:
  features:    Tensor [3, freq_bins]  (far | mic | residual magnitude spectra)
  target_mask: Tensor [1, freq_bins]  ideal ratio mask in [0, 1]
"""

import os
import random
from pathlib import Path
from typing import Literal

import numpy as np
import torch
from torch.utils.data import Dataset
import soundfile as sf

# Optional: librosa for resampling
try:
    import librosa
    HAS_LIBROSA = True
except ImportError:
    HAS_LIBROSA = False


class AECDataset(Dataset):
    def __init__(self,
                 root:      str,
                 split:     Literal["train", "val", "test"] = "train",
                 sample_rate: int  = 16000,
                 frame_size:  int  = 160,    # 10 ms @ 16 kHz
                 fft_size:    int  = 512,
                 hop_size:    int  = 128,
                 freq_bins:   int  = 257,
                 frames_per_sample: int = 1):
        super().__init__()
        self.root      = Path(root) / split
        self.sr        = sample_rate
        self.frame_size = frame_size
        self.fft_size  = fft_size
        self.hop_size  = hop_size
        self.freq_bins = freq_bins
        self.fps       = frames_per_sample  # frames stacked per sample

        # Discover file pairs
        far_dir = self.root / "farend_speech"
        mic_dir = self.root / "nearend_mic"
        ref_dir = self.root / "nearend_speech"

        if not far_dir.exists():
            raise FileNotFoundError(f"Dataset path not found: {far_dir}")

        self.pairs = []
        for far_path in sorted(far_dir.glob("*.wav")):
            stem = far_path.stem
            mic_path = mic_dir / f"{stem}.wav"
            ref_path = ref_dir / f"{stem}.wav"
            if mic_path.exists() and ref_path.exists():
                self.pairs.append((far_path, mic_path, ref_path))

        if not self.pairs:
            raise RuntimeError(f"No complete file triplets found in {self.root}")

        # Pre-compute total STFT frame count across all files
        self._index: list[tuple[int, int]] = []  # (pair_idx, frame_idx)
        self._frame_counts: list[int] = []
        for i, (far, _, _) in enumerate(self.pairs):
            info = sf.info(str(far))
            n_frames = max(0, (info.frames - self.fft_size) // self.hop_size)
            self._frame_counts.append(n_frames)
            for f in range(n_frames):
                self._index.append((i, f))

    # ── Helpers ────────────────────────────────────────────────────────────────

    def _load_segment(self, path: Path, start_sample: int) -> np.ndarray:
        """Load fft_size samples starting at start_sample."""
        data, sr = sf.read(str(path), start=start_sample,
                           frames=self.fft_size, dtype="float32",
                           always_2d=False)
        if sr != self.sr and HAS_LIBROSA:
            data = librosa.resample(data, orig_sr=sr, target_sr=self.sr)
        # Pad if short
        if len(data) < self.fft_size:
            data = np.pad(data, (0, self.fft_size - len(data)))
        return data

    @staticmethod
    def _stft_magnitude(signal: np.ndarray, fft_size: int) -> np.ndarray:
        window  = np.hanning(fft_size).astype(np.float32)
        frame   = signal * window
        spec    = np.fft.rfft(frame, n=fft_size)
        return np.abs(spec).astype(np.float32)   # [freq_bins]

    @staticmethod
    def _ideal_ratio_mask(clean_mag: np.ndarray,
                          noisy_mag: np.ndarray,
                          eps: float = 1e-8) -> np.ndarray:
        """Ideal Ratio Mask: |clean| / (|clean| + |noise|)."""
        noise_mag = np.maximum(noisy_mag - clean_mag, 0.0)
        return clean_mag / (clean_mag + noise_mag + eps)

    # ── Dataset API ────────────────────────────────────────────────────────────

    def __len__(self) -> int:
        return len(self._index)

    def __getitem__(self, idx: int):
        pair_idx, frame_idx = self._index[idx]
        far_path, mic_path, ref_path = self.pairs[pair_idx]
        start = frame_idx * self.hop_size

        far_sig = self._load_segment(far_path, start)
        mic_sig = self._load_segment(mic_path, start)
        ref_sig = self._load_segment(ref_path, start)

        far_mag = self._stft_magnitude(far_sig, self.fft_size)
        mic_mag = self._stft_magnitude(mic_sig, self.fft_size)
        ref_mag = self._stft_magnitude(ref_sig, self.fft_size)

        # Residual estimate: mic - far (simple approximation at training time)
        residual_mag = np.maximum(mic_mag - far_mag, 0.0)

        # Normalise each spectrum to unit max to ease training
        def norm(x: np.ndarray) -> np.ndarray:
            m = x.max()
            return x / (m + 1e-8)

        features = np.stack([norm(far_mag), norm(mic_mag),
                              norm(residual_mag)], axis=0)   # [3, freq_bins]

        target_mask = self._ideal_ratio_mask(ref_mag, mic_mag)[np.newaxis]  # [1, freq_bins]

        return (torch.from_numpy(features),
                torch.from_numpy(target_mask.astype(np.float32)))
