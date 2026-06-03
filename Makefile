# NeuralEcho — convenience Makefile
# Wraps CMake, Cargo, and Python commands.
#
# Usage:
#   make configure    — CMake configure (Release)
#   make build        — CMake build
#   make test         — Run C++ unit tests
#   make bench        — Run benchmark suite
#   make rust         — Build Rust workspace only
#   make lint-rust    — cargo fmt + clippy
#   make train        — Train NeuralEchoNet (requires GPU + dataset)
#   make export       — Export ONNX model
#   make eval-erle    — Evaluate ERLE on AEC Challenge test set
#   make clean        — Remove build/

VCPKG_ROOT   ?= $(HOME)/vcpkg
BUILD_DIR    ?= build
BUILD_TYPE   ?= Release
DATASET_DIR  ?= datasets/aec_challenge
CKPT         ?= lightning_logs/version_0/checkpoints/last.ckpt
MODEL_OUT    ?= models/neural_echo_net_int8.onnx
NPROC        := $(shell nproc 2>/dev/null || sysctl -n hw.logicalcpu 2>/dev/null || echo 4)

.PHONY: all configure build test bench rust lint-rust clean \
        train export eval-erle eval-pesq docker-build

all: build

# ── C++ ────────────────────────────────────────────────────────────────────────
configure:
	cmake -B $(BUILD_DIR) \
	      -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) \
	      -DCMAKE_TOOLCHAIN_FILE=$(VCPKG_ROOT)/scripts/buildsystems/vcpkg.cmake \
	      -DNEURAL_ECHO_BUILD_TESTS=ON \
	      -DNEURAL_ECHO_BUILD_BENCH=ON

build: configure
	cmake --build $(BUILD_DIR) --parallel $(NPROC)

test: build
	cd $(BUILD_DIR) && ctest --output-on-failure --parallel $(NPROC)

bench: build
	$(BUILD_DIR)/neural_echo_bench \
	    --benchmark_format=json \
	    --benchmark_out=$(BUILD_DIR)/bench_results.json \
	    --benchmark_repetitions=5
	@echo "Results written to $(BUILD_DIR)/bench_results.json"

# ── Rust ───────────────────────────────────────────────────────────────────────
rust:
	cd rust && cargo build --release

lint-rust:
	cd rust && cargo fmt --all
	cd rust && cargo clippy --all-targets --all-features -- -D warnings

test-rust:
	cd rust && cargo test --all

# ── Python ML ──────────────────────────────────────────────────────────────────
train:
	python ml/training/train.py \
	    --data_dir $(DATASET_DIR) \
	    --epochs 100 \
	    --batch_size 32

export:
	python ml/training/export_onnx.py \
	    --checkpoint $(CKPT) \
	    --output $(MODEL_OUT)

eval-erle:
	python ml/evaluation/eval_erle.py \
	    --mic_dir $(DATASET_DIR)/test/nearend_mic \
	    --out_dir results/processed \
	    --save_csv results/erle_results.csv

eval-pesq:
	python ml/evaluation/eval_pesq_stoi.py \
	    --ref_dir $(DATASET_DIR)/test/nearend_speech \
	    --deg_dir results/processed \
	    --save_csv results/pesq_stoi_results.csv

# ── Integration tests ─────────────────────────────────────────────────────────
test-integration:
	pytest tests/integration/ -v --build-dir $(BUILD_DIR)

test-model:
	pytest tests/integration/test_model_inference.py -v

# ── Docker ────────────────────────────────────────────────────────────────────
docker-build:
	docker build -t neural-echo:latest .

docker-test:
	docker run --rm neural-echo:latest ./neural_echo_tests

# ── Misc ──────────────────────────────────────────────────────────────────────
clean:
	rm -rf $(BUILD_DIR)
	cd rust && cargo clean

format-cpp:
	find dsp apps tests benchmarks -name "*.cpp" -o -name "*.h" | \
	    xargs clang-format -i --style=file

help:
	@echo "NeuralEcho build targets:"
	@echo "  configure      CMake configure"
	@echo "  build          CMake build (includes Rust)"
	@echo "  test           C++ unit tests (ctest)"
	@echo "  bench          Google Benchmark suite"
	@echo "  rust           Cargo build only"
	@echo "  lint-rust      cargo fmt + clippy"
	@echo "  train          Train NeuralEchoNet"
	@echo "  export         Export ONNX model"
	@echo "  eval-erle      ERLE evaluation"
	@echo "  eval-pesq      PESQ + STOI evaluation"
	@echo "  docker-build   Build Docker image"
	@echo "  clean          Remove build artifacts"
