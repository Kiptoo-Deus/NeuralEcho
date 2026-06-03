# NeuralEcho — Week-by-Week Implementation Roadmap

Total duration: **32 weeks (8 months)**

---

## Phase 1 — DSP Foundation (Weeks 1–8)

### Weeks 1–2 · Repository & FFT Engine
- [ ] Initialise Git repository, CMake root, vcpkg manifest, `.gitignore`
- [ ] Rust workspace skeleton (`Cargo.toml`, all crate directories)
- [ ] GitHub Actions CI pipelines (Linux, macOS)
- [ ] `FFTEngine` — FFTW3 STFT / ISTFT, Hann / Hamming / Blackman windows
- [ ] Google Test suite: `test_fft_engine.cpp` — round-trip, impulse, bin count
- [ ] Google Benchmark: `bench_fft.cpp` — sizes 256 / 512 / 1024 / 2048

**Deliverable:** Green CI, FFT tests pass, benchmark baseline recorded.

---

### Weeks 3–4 · NLMS Adaptive Filter
- [ ] `NLMSFilter` — circular delay line, weight update, leakage
- [ ] xsimd SIMD dot product path (`NE_HAVE_XSIMD`)
- [ ] `StepSizeScheduler` — dynamic mu adaptation
- [ ] Tests: convergence on synthetic echo, freeze/resume, reset
- [ ] Benchmark: scalar vs SIMD throughput for 128–2048 taps

**Deliverable:** NLMS achieves > 20 dB ERLE on synthetic test signal.

---

### Weeks 5–6 · Double-Talk Detector & Residual Echo Estimator
- [ ] `DoubleTalkDetector` — Geigel energy ratio + spectral coherence
- [ ] `ResidualEchoEstimator` — PSD over-subtraction with frequency smoothing
- [ ] Tests: far-end only / near-end only / double-talk / silence cases
- [ ] Integration: DTD freeze signal wired into NLMS adaptation gate

**Deliverable:** DTD correctly classifies talk states on synthetic test vectors.

---

### Weeks 7–8 · VAD, AGC & AEC Pipeline Facade
- [ ] `VoiceActivityDetector` — energy backend + WebRTC VAD C binding stub
- [ ] `AutomaticGainControl` — attack/release envelope + gain smoothing
- [ ] `MetricsCollector` — thread-safe ERLE, RTF accumulation
- [ ] `AECPipeline` — facade wiring all DSP modules
- [ ] Integration test: full pipeline ERLE > 10 dB regression gate
- [ ] Benchmark: `bench_pipeline.cpp` — P99 latency target < 8 ms

**Deliverable:** Complete DSP chain with automated latency regression gate.

---

## Phase 2 — Neural Network (Weeks 9–12)

### Weeks 9–10 · Model Training
- [ ] `NeuralEchoNet` — Conv1d encoder / GRU / Conv1d decoder
- [ ] `AECDataset` — AEC Challenge file loader, STFT feature extraction, IRM targets
- [ ] `HybridLoss` — MaskMSE + SI-SNR proxy
- [ ] `train.py` — Lightning trainer, AdamW + OneCycleLR, EarlyStopping
- [ ] Initial training run on RTX 4070 (mixed precision, 100 epochs)

**Deliverable:** Trained checkpoint with val/loss < 0.05 and PESQ > 3.0.

---

### Weeks 11–12 · ONNX Export & Evaluation Pipeline
- [ ] `export_onnx.py` — FP32 export then INT8 dynamic quantisation
- [ ] `eval_erle.py`, `eval_pesq_stoi.py` — batch evaluation scripts
- [ ] Evaluate quantised model on AEC Challenge test set
- [ ] Model size < 250 KB, inference < 2 ms verified on CPU

**Deliverable:** `neural_echo_net_int8.onnx` in `models/`; PESQ > 3.5, STOI > 0.92.

---

## Phase 3 — Rust Inference Runtime (Weeks 13–16)

### Weeks 13–14 · Core Rust Crates
- [ ] `neural_echo_config` — TOML schema + `load_or_default()`
- [ ] `neural_echo_buffers` — `SpscRing<T>` (crossbeam), `FramePool`
- [ ] `neural_echo_inference` — `NeuralEchoModel` wrapping `ort::Session`
- [ ] `neural_echo_features` — normalisation, spectrum stacking
- [ ] Unit tests: ring buffer push/pop under contention, model load smoke test

**Deliverable:** Rust workspace compiles; ONNX model loads and produces masks.

---

### Weeks 15–16 · FFI Layer & C++ Integration
- [ ] `neural_echo_ffi` — `ne_context_create / destroy / process_frame` FFI
- [ ] `cbindgen` build.rs — auto-generates `neural_echo_ffi.h`
- [ ] CMake `add_custom_command` builds Rust static lib + links to C++
- [ ] `NeuralBridge` C++ wrapper class in DSP engine
- [ ] End-to-end latency budget test: DSP + neural < 10 ms combined
- [ ] Windows CI pipeline added

**Deliverable:** Single binary calling Rust inference from C++ audio thread.

---

## Phase 4 — Qt 6 Dashboard (Weeks 17–22)

### Weeks 17–18 · Core Dashboard & Spectrogram
- [ ] `AppController` — Qt signals wiring pipeline metrics to QML
- [ ] `SidebarNav`, `ThemeConstants`, `StatusBar` components
- [ ] `DashboardPage` — waveform plots, metric cards, start/stop control
- [ ] `SpectrogramPage` — scrolling Canvas spectrogram (before/after/residual)
- [ ] 25 Hz refresh timer → `frameReady` signal

**Deliverable:** Live waveform and spectrogram running with real audio.

---

### Weeks 19–20 · Metrics, Experiments & Inspector
- [ ] `MetricsPage` — Qt Charts line charts for ERLE / PESQ / STOI / RTF, CSV export
- [ ] `ExperimentsPage` — FileDialog dataset picker, batch evaluation runner,
  auto-generated PDF/Markdown report
- [ ] `ModelInspectorPage` — ONNX layer TreeView, activation heatmap Canvas,
  per-layer timing breakdown

**Deliverable:** All five dashboard pages functional.

---

### Weeks 21–22 · Optimisation Pass
- [ ] Profile NLMS SIMD path with perf / Instruments
- [ ] ONNX Runtime thread-affinity and arena allocator tuning
- [ ] Qt Quick rendering profiling — eliminate unnecessary repaints
- [ ] Target validation: CPU < 10 %, RAM < 200 MB on reference machine (i5-12400)

**Deliverable:** Performance targets met; profiling report committed to `docs/`.

---

## Phase 5 — Benchmarking Suite (Weeks 23–24)

- [ ] Full Google Benchmark suite with JSON output + CI regression gate
- [ ] Python scripts run AEC Challenge full test set → ERLE / PESQ / STOI tables
- [ ] Baseline comparison vs WebRTC AEC-M and SpeexDSP
- [ ] Cross-platform benchmark consistency verified (Linux / macOS / Windows)
- [ ] `docs/benchmark_results.md` auto-generated from CI artefacts

**Deliverable:** Publication-ready benchmark tables and plots.

---

## Phase 6 — Papers (Weeks 25–30)

### Weeks 25–26 · Paper 1 — Hybrid AEC Architecture
- Hybrid adaptive + neural approach description
- Experimental results vs baselines (ERLE, PESQ, STOI tables)
- System latency breakdown figure
- Target: ICASSP / Interspeech

### Weeks 27–28 · Paper 2 — Rust Runtime
- Rust inference runtime design and lock-free buffer analysis
- Quantisation accuracy vs latency trade-off curves
- Cross-platform latency consistency results
- Target: IEEE Signal Processing Letters

### Weeks 29–30 · Paper 3 — Cross-Platform DSP Framework
- Full system architecture and Qt dashboard design patterns
- Reproducibility: CI-executed benchmark regeneration workflow
- Target: IEEE ICME / EUSIPCO

---

## Phase 7 — Release & Submission (Weeks 31–32)

- [ ] Final paper revisions and submission
- [ ] GitHub release v1.0.0: tag, changelog, release notes
- [ ] Documentation site: Doxygen API docs + mdBook architecture guide
- [ ] Demo video recorded and linked from README
- [ ] Open-source announcement (HN, Reddit r/MachineLearning, r/dsp)
- [ ] Buffer for reviewer feedback from earlier paper submissions

---

## Milestone Summary

| Week | Milestone |
|------|-----------|
| 2    | FFT engine + CI green |
| 4    | NLMS > 20 dB ERLE |
| 8    | Full DSP pipeline + latency gate |
| 10   | Neural model trained |
| 12   | ONNX INT8 model evaluated |
| 16   | Rust FFI linked to C++ |
| 22   | All Qt pages + perf targets met |
| 24   | Benchmark tables complete |
| 30   | Three papers submitted |
| 32   | v1.0.0 public release |
