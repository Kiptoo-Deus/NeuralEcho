"""
PESQ + STOI evaluation script.
================================
Usage:
  python eval_pesq_stoi.py \
      --ref_dir  /data/nearend_speech \
      --deg_dir  /data/processed      \
      --sample_rate 16000
"""

import argparse
from pathlib import Path
import numpy as np
import soundfile as sf

try:
    from pesq import pesq as compute_pesq
    HAS_PESQ = True
except ImportError:
    HAS_PESQ = False
    print("[WARNING] pesq not installed. Run: pip install pesq")

try:
    from pystoi import stoi as compute_stoi
    HAS_STOI = True
except ImportError:
    HAS_STOI = False
    print("[WARNING] pystoi not installed. Run: pip install pystoi")


def evaluate(ref_dir: Path, deg_dir: Path, sr: int) -> list[dict]:
    rows = []
    for ref_path in sorted(ref_dir.glob("*.wav")):
        deg_path = deg_dir / ref_path.name
        if not deg_path.exists():
            continue

        ref_sig, ref_sr = sf.read(str(ref_path), dtype="float32", always_2d=False)
        deg_sig, deg_sr = sf.read(str(deg_path), dtype="float32", always_2d=False)

        # Align lengths
        n = min(len(ref_sig), len(deg_sig))
        ref_sig = ref_sig[:n]
        deg_sig = deg_sig[:n]

        row: dict = {"file": ref_path.stem}

        if HAS_PESQ:
            try:
                mode = "nb" if sr == 8000 else "wb"
                row["pesq"] = float(compute_pesq(sr, ref_sig, deg_sig, mode))
            except Exception as e:
                row["pesq"] = float("nan")
                print(f"  PESQ error on {ref_path.name}: {e}")

        if HAS_STOI:
            try:
                row["stoi"] = float(compute_stoi(ref_sig, deg_sig, sr, extended=False))
            except Exception as e:
                row["stoi"] = float("nan")
                print(f"  STOI error on {ref_path.name}: {e}")

        rows.append(row)
    return rows


def print_summary(rows: list[dict]):
    has_pesq = any("pesq" in r for r in rows)
    has_stoi = any("stoi" in r for r in rows)

    header = f"{'File':<40}"
    if has_pesq: header += f"  {'PESQ':>8}"
    if has_stoi: header += f"  {'STOI':>8}"
    print(f"\n{header}")
    print("-" * len(header))

    for r in rows:
        line = f"{r['file']:<40}"
        if has_pesq: line += f"  {r.get('pesq', float('nan')):>8.3f}"
        if has_stoi: line += f"  {r.get('stoi', float('nan')):>8.4f}"
        print(line)

    print("-" * len(header))
    if has_pesq:
        vals = [r["pesq"] for r in rows if not np.isnan(r.get("pesq", float("nan")))]
        print(f"{'Mean PESQ':.<40}  {np.mean(vals):>8.3f}")
    if has_stoi:
        vals = [r["stoi"] for r in rows if not np.isnan(r.get("stoi", float("nan")))]
        print(f"{'Mean STOI':.<40}  {np.mean(vals):>8.4f}")


def save_csv(rows: list[dict], path: str):
    if not rows:
        return
    keys = list(rows[0].keys())
    with open(path, "w") as f:
        f.write(",".join(keys) + "\n")
        for r in rows:
            f.write(",".join(str(r.get(k, "")) for k in keys) + "\n")
    print(f"\nResults saved → {path}")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--ref_dir",     required=True)
    parser.add_argument("--deg_dir",     required=True)
    parser.add_argument("--sample_rate", type=int, default=16000)
    parser.add_argument("--save_csv",    default="pesq_stoi_results.csv")
    args = parser.parse_args()

    rows = evaluate(Path(args.ref_dir), Path(args.deg_dir), args.sample_rate)
    print_summary(rows)
    save_csv(rows, args.save_csv)


if __name__ == "__main__":
    main()
