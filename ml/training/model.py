"""
NeuralEchoNet — Convolutional-Recurrent Residual Echo Suppressor
================================================================
Architecture:
  Input  [B, 3, F]   (far-end | mic | residual magnitude spectra)
  Encoder: 3 × Conv1d + PReLU
  GRU:     2 layers, hidden=256
  Decoder: 3 × Conv1d + PReLU → Sigmoid mask
  Output [B, 1, F]   spectral suppression mask in [0, 1]
"""

import torch
import torch.nn as nn


class ConvBlock(nn.Module):
    def __init__(self, in_ch: int, out_ch: int, kernel: int = 3):
        super().__init__()
        self.conv = nn.Conv1d(in_ch, out_ch, kernel,
                              padding=kernel // 2, bias=True)
        self.act  = nn.PReLU(num_parameters=out_ch)

    def forward(self, x: torch.Tensor) -> torch.Tensor:
        return self.act(self.conv(x))


class NeuralEchoNet(nn.Module):
    """
    Hybrid echo suppression network.

    Args:
        freq_bins:    Number of frequency bins (N_FFT/2 + 1). Default 257.
        gru_hidden:   GRU hidden state size.          Default 256.
        gru_layers:   Number of stacked GRU layers.   Default 2.
    """

    def __init__(self,
                 freq_bins:  int = 257,
                 gru_hidden: int = 256,
                 gru_layers: int = 2):
        super().__init__()
        self.freq_bins  = freq_bins
        self.gru_hidden = gru_hidden

        # Encoder  [B, 3, F] → [B, 128, F]
        self.encoder = nn.Sequential(
            ConvBlock(3,   32,  3),
            ConvBlock(32,  64,  3),
            ConvBlock(64,  128, 3),
        )

        # Recurrent  [B, F, 128] → [B, F, 256]
        self.gru = nn.GRU(
            input_size=128,
            hidden_size=gru_hidden,
            num_layers=gru_layers,
            batch_first=True,
            dropout=0.1 if gru_layers > 1 else 0.0,
        )

        # Decoder  [B, 256, F] → [B, 1, F]
        self.decoder = nn.Sequential(
            ConvBlock(gru_hidden, 128, 3),
            ConvBlock(128,        32,  3),
            nn.Conv1d(32, 1, 1),
            nn.Sigmoid(),
        )

    def forward(self,
                x: torch.Tensor,
                h: torch.Tensor | None = None
                ) -> tuple[torch.Tensor, torch.Tensor]:
        """
        Args:
            x: [B, 3, F]  — concatenated feature spectra
            h: optional GRU hidden state for streaming inference

        Returns:
            mask: [B, 1, F]  in [0, 1]
            h:    updated GRU hidden state
        """
        # Encode
        enc = self.encoder(x)               # [B, 128, F]

        # Reshape for GRU: sequence along frequency axis
        enc = enc.permute(0, 2, 1)          # [B, F, 128]
        gru_out, h = self.gru(enc, h)       # [B, F, 256]
        gru_out = gru_out.permute(0, 2, 1)  # [B, 256, F]

        # Decode to mask
        mask = self.decoder(gru_out)        # [B, 1, F]
        return mask, h

    @torch.no_grad()
    def infer(self, x: torch.Tensor) -> torch.Tensor:
        """Single-frame inference (no gradient tracking)."""
        mask, _ = self.forward(x)
        return mask


def build_model(freq_bins: int = 257) -> NeuralEchoNet:
    return NeuralEchoNet(freq_bins=freq_bins)


if __name__ == "__main__":
    model = build_model()
    x     = torch.randn(1, 3, 257)
    mask, _ = model(x)
    print(f"Input:  {x.shape}")
    print(f"Output: {mask.shape}")
    total = sum(p.numel() for p in model.parameters())
    print(f"Parameters: {total:,}")
