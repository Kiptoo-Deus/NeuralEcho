"""
NeuralEchoNet training script
==============================
Usage:
  python train.py --data_dir /path/to/aec_challenge --epochs 100

Requires:
  pip install torch lightning wandb pesq pystoi
"""

import argparse
import torch
import lightning as L
from torch.utils.data import DataLoader
from model   import NeuralEchoNet
from dataset import AECDataset
from losses  import HybridLoss


class NeuralEchoLitModule(L.LightningModule):
    def __init__(self, freq_bins: int = 257, lr: float = 3e-4):
        super().__init__()
        self.save_hyperparameters()
        self.model = NeuralEchoNet(freq_bins=freq_bins)
        self.loss  = HybridLoss()

    def forward(self, x):
        mask, _ = self.model(x)
        return mask

    def _shared_step(self, batch):
        features, target_mask = batch
        pred_mask = self(features)
        return self.loss(pred_mask, target_mask, features[:, 1:2, :])

    def training_step(self, batch, _):
        loss = self._shared_step(batch)
        self.log("train/loss", loss, prog_bar=True)
        return loss

    def validation_step(self, batch, _):
        loss = self._shared_step(batch)
        self.log("val/loss", loss, prog_bar=True, sync_dist=True)

    def configure_optimizers(self):
        opt = torch.optim.AdamW(
            self.parameters(), lr=self.hparams.lr, weight_decay=1e-4)
        sched = torch.optim.lr_scheduler.OneCycleLR(
            opt,
            max_lr=self.hparams.lr,
            total_steps=self.trainer.estimated_stepping_batches,
        )
        return {"optimizer": opt,
                "lr_scheduler": {"scheduler": sched, "interval": "step"}}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--data_dir",  required=True)
    parser.add_argument("--epochs",    type=int, default=100)
    parser.add_argument("--batch_size",type=int, default=32)
    parser.add_argument("--freq_bins", type=int, default=257)
    parser.add_argument("--lr",        type=float, default=3e-4)
    parser.add_argument("--num_workers", type=int, default=4)
    args = parser.parse_args()

    train_ds = AECDataset(args.data_dir, split="train", freq_bins=args.freq_bins)
    val_ds   = AECDataset(args.data_dir, split="val",   freq_bins=args.freq_bins)

    train_dl = DataLoader(train_ds, batch_size=args.batch_size,
                          shuffle=True,  num_workers=args.num_workers)
    val_dl   = DataLoader(val_ds,   batch_size=args.batch_size,
                          shuffle=False, num_workers=args.num_workers)

    module = NeuralEchoLitModule(freq_bins=args.freq_bins, lr=args.lr)

    trainer = L.Trainer(
        max_epochs=args.epochs,
        precision="16-mixed",
        gradient_clip_val=1.0,
        log_every_n_steps=20,
        callbacks=[
            L.pytorch.callbacks.EarlyStopping(
                monitor="val/loss", patience=10, mode="min"),
            L.pytorch.callbacks.ModelCheckpoint(
                monitor="val/loss", save_top_k=3,
                filename="neuralecho-{epoch:02d}-{val_loss:.4f}"),
        ],
    )
    trainer.fit(module, train_dl, val_dl)


if __name__ == "__main__":
    main()
