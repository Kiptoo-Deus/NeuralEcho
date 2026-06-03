# NeuralEcho

**Hybrid Rust + C++ Acoustic Echo Cancellation Research Platform**

[![CI Linux](https://github.com/joelkiptoo/NeuralEcho/actions/workflows/ci_linux.yml/badge.svg)](https://github.com/joelkiptoo/NeuralEcho/actions)
[![CI macOS](https://github.com/joelkiptoo/NeuralEcho/actions/workflows/ci_macos.yml/badge.svg)](https://github.com/joelkiptoo/NeuralEcho/actions)
[![CI Windows](https://github.com/joelkiptoo/NeuralEcho/actions/workflows/ci_windows.yml/badge.svg)](https://github.com/joelkiptoo/NeuralEcho/actions)
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)

---

## Overview

NeuralEcho is an open-source, cross-platform Acoustic Echo Cancellation (AEC)
platform that combines a classical NLMS adaptive pre-filter (C++20, SIMD-accelerated)
with a convolutional-recurrent neural residual echo suppressor (ONNX Runtime via Rust).
A Qt 6 / Qt Quick dashboard provides real-time waveform and spectrogram visualisation,
session metrics, batch experiment management, and a model inspector.

### Performance Targets

| Metric | Target | Notes |
|---|---|---|
| ERLE | > 35 dB | Echo Return Loss Enhancement |
| End-to-end latency | < 10 ms | DSP + neural inference combined |
| Neural inference | < 2 ms | INT8 ONNX, single CPU core |
| PESQ | > 3.5 | ITU-T P.862.2 wideband |
| STOI | > 0.92 | Short-Time Objective Intelligibility |
| CPU usage | < 10 % | Single i5-class core |
| RAM | < 200 MB | Full pipeline + Qt dashboard |

---

## Architecture

```
Qt 6 Dashboard (QML)
       │  Qt Signals
App Controller (C++)
       │                     │
DSP Engine (C++20)     Neural Runtime (Rust)
  FFT · NLMS · DTD      ONNX Inference · GRU
  VAD · AGC · Metrics   Lock-free Queues
       │                     │
       └──── Zero-copy shared ring buffer ────┘
                      │
            PortAudio / RtAudio
```

---

## Repository Structure

```
NeuralEcho/
├── apps/qt_dashboard/       Qt 6 / QML dashboard
├── dsp/                     C++20 DSP engine (FFT, NLMS, DTD, AGC, VAD)
├── rust/                    Rust workspace (inference, FFI, buffers)
├── ml/                      Python training & evaluation
│   ├── training/            model.py, train.py, dataset.py, losses.py
│   └── evaluation/          eval_erle.py, eval_pesq_stoi.py
├── benchmarks/              Google Benchmark suites
├── tests/                   Google Test unit tests
├── .github/workflows/       CI pipelines (Linux, macOS, Windows)
├── CMakeLists.txt
├── vcpkg.json
├── neural_echo.toml         Runtime configuration
└── requirements.txt         Python ML dependencies
```

---

## Building

### Prerequisites

| Tool | Version |
|---|---|
| CMake | ≥ 3.27 |
| C++ compiler | GCC 13 / Clang 17 / MSVC 2022 |
| Rust | stable (≥ 1.78) |
| vcpkg | latest |
| Qt 6 | ≥ 6.7 (optional, for dashboard) |
| Python | 3.11+ (optional, for ML) |

### Linux / macOS

```bash
# 1. Clone
git clone https://github.com/joelkiptoo/NeuralEcho.git
cd NeuralEcho

# 2. Bootstrap vcpkg (first time only)
git clone https://github.com/microsoft/vcpkg.git ~/vcpkg
~/vcpkg/bootstrap-vcpkg.sh

# 3. Configure
cmake -B build \
  -DCMAKE_TOOLCHAIN_FILE=~/vcpkg/scripts/buildsystems/vcpkg.cmake \
  -DCMAKE_BUILD_TYPE=Release

# 4. Build (Rust runtime is built automatically via cargo)
cmake --build build --parallel

# 5. Run tests
cd build && ctest --output-on-failure
```

### Windows (PowerShell)

```powershell
git clone https://github.com/joelkiptoo/NeuralEcho.git
cd NeuralEcho

# Bootstrap vcpkg
git clone https://github.com/microsoft/vcpkg.git C:\vcpkg
C:\vcpkg\bootstrap-vcpkg.bat

cmake -B build `
  -DCMAKE_TOOLCHAIN_FILE="C:\vcpkg\scripts\buildsystems\vcpkg.cmake" `
  -DNEURAL_ECHO_BUILD_QT=OFF
cmake --build build --config Release
cd build && ctest -C Release --output-on-failure
```

### Docker

```bash
docker build -t neural-echo .
docker run --rm neural-echo ./neural_echo_tests
```

---

## Machine Learning

### Setup

```bash
pip install -r requirements.txt
```

### Download Dataset

```bash
bash ml/datasets/download_aec_challenge.sh --dest datasets/aec_challenge
```

### Train

```bash
python ml/training/train.py \
    --data_dir datasets/aec_challenge \
    --epochs 100 \
    --batch_size 32
```

### Export to ONNX

```bash
python ml/training/export_onnx.py \
    --checkpoint lightning_logs/.../best.ckpt \
    --output models/neural_echo_net_int8.onnx
```

### Evaluate

```bash
# ERLE
python ml/evaluation/eval_erle.py \
    --mic_dir datasets/aec_challenge/test/nearend_mic \
    --out_dir results/processed

# PESQ + STOI
python ml/evaluation/eval_pesq_stoi.py \
    --ref_dir datasets/aec_challenge/test/nearend_speech \
    --deg_dir results/processed
```

---

## Benchmarks

```bash
./build/neural_echo_bench \
    --benchmark_format=json \
    --benchmark_out=bench_results.json \
    --benchmark_repetitions=5
```

Key benchmark targets:

| Suite | Measured | Regression Limit |
|---|---|---|
| `BM_STFT` | FFT throughput (frames/s) | −5 % vs baseline |
| `BM_NLMSFilter` | Filter throughput + SIMD speedup | −5 % vs baseline |
| `BM_PipelineLatencyHistogram` | P99 end-to-end latency | < 8 ms |
| `BM_PipelineThroughput` | Samples/second | — |

---

## Neural Model Summary

```
NeuralEchoNet
  Input  [1, 3, 257]   (far-end | mic | residual spectra)
  Encoder  Conv1d × 3 + PReLU   →  [1, 128, 257]
  GRU      2 layers, hidden=256  →  [1, 256, 257]
  Decoder  Conv1d × 3 + Sigmoid  →  [1, 1,   257]
  Output   Suppression mask in [0, 1]^257

  Parameters : ~800 K (FP32)
  Quant size  : ~200 KB (INT8)
  Inference   : < 2 ms (ONNX Runtime, CPU)
```

---

## Roadmap

See the [week-by-week implementation roadmap](docs/ROADMAP.md) for the full
32-week delivery schedule across DSP engine, neural runtime, Qt dashboard,
benchmarking, and paper writing phases.

---

## License

MIT © 2024 Joel Kiptoo
