"""
Export a trained NeuralEchoNet checkpoint to ONNX with INT8 quantisation.

Usage:
  python export_onnx.py \
      --checkpoint lightning_logs/.../checkpoints/best.ckpt \
      --output     ../../models/neural_echo_net_int8.onnx \
      --freq_bins  257
"""

import argparse
from pathlib import Path

import torch
from onnxruntime.quantization import quantize_dynamic, QuantType

from model import NeuralEchoNet
from train import NeuralEchoLitModule


def export(checkpoint: str, output: str, freq_bins: int) -> None:
    out_path    = Path(output)
    float_path  = out_path.with_suffix(".float32.onnx")
    out_path.parent.mkdir(parents=True, exist_ok=True)

    # Load Lightning checkpoint
    print(f"Loading checkpoint: {checkpoint}")
    lit = NeuralEchoLitModule.load_from_checkpoint(checkpoint)
    model: NeuralEchoNet = lit.model
    model.eval()

    # Dummy input  [1, 3, freq_bins]
    dummy_input = torch.zeros(1, 3, freq_bins, dtype=torch.float32)

    # Export FP32 ONNX
    print(f"Exporting FP32 ONNX → {float_path}")
    torch.onnx.export(
        model,
        (dummy_input,),
        str(float_path),
        input_names=["input"],
        output_names=["output"],
        dynamic_axes={"input":  {0: "batch"}, "output": {0: "batch"}},
        opset_version=17,
        do_constant_folding=True,
    )

    # INT8 dynamic quantisation
    print(f"Quantising INT8 → {out_path}")
    quantize_dynamic(
        str(float_path),
        str(out_path),
        weight_type=QuantType.QInt8,
    )

    # Size comparison
    fp32_kb = float_path.stat().st_size / 1024
    int8_kb = out_path.stat().st_size  / 1024
    print(f"FP32: {fp32_kb:.0f} KB   INT8: {int8_kb:.0f} KB  "
          f"({int8_kb/fp32_kb*100:.0f}% of original)")
    print("Export complete.")


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--checkpoint", required=True,
                        help="Path to Lightning .ckpt file")
    parser.add_argument("--output",
                        default="../../models/neural_echo_net_int8.onnx",
                        help="Output ONNX path (INT8)")
    parser.add_argument("--freq_bins", type=int, default=257)
    args = parser.parse_args()
    export(args.checkpoint, args.output, args.freq_bins)
