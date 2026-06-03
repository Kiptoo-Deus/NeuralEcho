"""
Loss functions for NeuralEchoNet training.

HybridLoss combines:
  1. MSE on the suppression mask
  2. Scale-Invariant SNR (SI-SNR) proxy on the enhanced spectrum
"""

import torch
import torch.nn as nn
import torch.nn.functional as F


class MaskMSELoss(nn.Module):
    """Mean-squared error between predicted and ideal ratio masks."""

    def forward(self, pred: torch.Tensor, target: torch.Tensor) -> torch.Tensor:
        return F.mse_loss(pred, target)


class SISnrLoss(nn.Module):
    """
    Scale-Invariant SNR loss (negated, so minimising = maximising SNR).
    Operates on magnitude spectra as a waveform-free training proxy.

    Args:
        pred_mask : [B, 1, F]  predicted suppression mask
        mic_spec  : [B, 1, F]  mic magnitude spectrum (noisy)
    """

    def forward(self, pred_mask: torch.Tensor, mic_spec: torch.Tensor) -> torch.Tensor:
        enhanced  = pred_mask * mic_spec                          # [B, 1, F]
        s_hat     = enhanced - enhanced.mean(dim=-1, keepdim=True)
        s_target  = mic_spec - mic_spec.mean(dim=-1, keepdim=True)

        dot       = (s_hat * s_target).sum(dim=-1, keepdim=True)
        s_sq      = (s_target ** 2).sum(dim=-1, keepdim=True) + 1e-8
        proj      = (dot / s_sq) * s_target

        noise     = s_hat - proj
        si_snr    = 10.0 * torch.log10(
            (proj ** 2).sum(dim=-1) / ((noise ** 2).sum(dim=-1) + 1e-8)
        )
        return -si_snr.mean()


class HybridLoss(nn.Module):
    """
    Weighted combination: alpha * MaskMSE + (1-alpha) * SI-SNR proxy.

    Args:
        mse_weight   : weight for the mask MSE term  (default 0.7)
        sisnr_weight : weight for the SI-SNR term     (default 0.3)
    """

    def __init__(self, mse_weight: float = 0.7, sisnr_weight: float = 0.3):
        super().__init__()
        self.mse_weight   = mse_weight
        self.sisnr_weight = sisnr_weight
        self._mse   = MaskMSELoss()
        self._sisnr = SISnrLoss()

    def forward(self,
                pred_mask:   torch.Tensor,
                target_mask: torch.Tensor,
                mic_spec:    torch.Tensor) -> torch.Tensor:
        mse   = self._mse(pred_mask, target_mask)
        sisnr = self._sisnr(pred_mask, mic_spec)
        return self.mse_weight * mse + self.sisnr_weight * sisnr
