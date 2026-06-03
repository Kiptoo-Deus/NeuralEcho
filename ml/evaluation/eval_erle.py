"""
ERLE (Echo Return Loss Enhancement) evaluation.
================================================
Usage:
  python eval_erle.py --mic_dir /data/nearend_mic \
                      --out_dir /data/processed   \
                      --ref_dir /data/nearend_speech
"""

import argparse
from pathlib import Path
import numpy as np
import soundfile as sf


def erle_db(mic: np.ndarray, out: np.ndarray, eps: float = 1e-9) -> float:
    """
    ERLE = 10 * log10( E[mic^2] / E[out^2] )
    Both arrays must have the same length.
    """
    mic_power = np.mean(mic ** 2) + eps
    out_power = np.mean(out ** 2) + eps
    return 10.0 * np.log10(mic_power / out_power)


def evaluate_directory(mic_dir: Path, out_dir: Path) -> dict:
    results = {}
    for mic_path in sorted(mic_dir.glob("*.wav")):
        out_path = out_dir / mic_path.name
        if not out_path.exists():
            print(f"  [SKIP] No output for {mic_path.name}")
            continue
        mic_sig, _ = sf.read(str(mic_path), dtype="float32", always_2d=False)
        out_sig, _ = sf.read(str(out_path), dtype="float32", always_2d=False)
        n = min(len(mic_sig), len(out_sig))
        score = erle_db(mic_sig[:n], out_sig[:n])
        results[mic_path.stem] = score
    return results


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--mic_dir", required=True)
    parser.add_argument("--out_dir", required=True)
    parser.add_argument("--save_csv", default="erle_results.csv")
    args = parser.parse_args()

    results = evaluate_directory(Path(args.mic_dir), Path(args.out_dir))
    values  = list(results.values())

    print(f"\n{'File':<40}  {'ERLE (dB)':>10}")
    print("-" * 54)
    for name, score in sorted(results.items()):
        print(f"{name:<40}  {score:>10.2f}")

    print("-" * 54)
    print(f"{'Mean':.<40}  {np.mean(values):>10.2f}")
    print(f"{'Std':.<40}  {np.std(values):>10.2f}")
    print(f"{'Min':.<40}  {np.min(values):>10.2f}")
    print(f"{'Max':.<40}  {np.max(values):>10.2f}")

    # CSV export
    with open(args.save_csv, "w") as f:
        f.write("file,erle_db\n")
        for name, score in sorted(results.items()):
            f.write(f"{name},{score:.4f}\n")
    print(f"\nResults saved → {args.save_csv}")


if __name__ == "__main__":
    main()
