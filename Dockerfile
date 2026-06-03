# ── NeuralEcho build environment ────────────────────────────────────────────
# Multi-stage Dockerfile:
#   Stage 1 (builder): Compiles C++ DSP + Rust runtime + tests
#   Stage 2 (runtime): Minimal image for running benchmarks / evaluation

FROM ubuntu:24.04 AS builder

# Avoid interactive prompts during package install
ENV DEBIAN_FRONTEND=noninteractive
ENV VCPKG_ROOT=/opt/vcpkg
ENV PATH="${PATH}:/root/.cargo/bin"

# ── System packages ───────────────────────────────────────────────────────────
RUN apt-get update -qq && \
    apt-get install -y --no-install-recommends \
        build-essential cmake ninja-build pkg-config git curl zip unzip tar \
        libfftw3-dev libeigen3-dev portaudio19-dev \
        libssl-dev ca-certificates \
        python3 python3-pip && \
    rm -rf /var/lib/apt/lists/*

# ── Rust ─────────────────────────────────────────────────────────────────────
RUN curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | \
    sh -s -- -y --default-toolchain stable
RUN . "$HOME/.cargo/env" && rustup component add clippy rustfmt

# ── vcpkg ────────────────────────────────────────────────────────────────────
RUN git clone --depth 1 https://github.com/microsoft/vcpkg.git $VCPKG_ROOT && \
    $VCPKG_ROOT/bootstrap-vcpkg.sh -disableMetrics

# ── Copy source ───────────────────────────────────────────────────────────────
WORKDIR /src
COPY . .

# ── Build Rust workspace ──────────────────────────────────────────────────────
WORKDIR /src/rust
RUN . "$HOME/.cargo/env" && \
    cargo build --release 2>&1 | tail -5

# ── Configure & build C++ (DSP + tests + benchmarks, no Qt) ──────────────────
WORKDIR /src
RUN cmake -B build \
      -G Ninja \
      -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake \
      -DNEURAL_ECHO_BUILD_QT=OFF \
      -DNEURAL_ECHO_BUILD_TESTS=ON \
      -DNEURAL_ECHO_BUILD_BENCH=ON && \
    cmake --build build --parallel $(nproc)

# ── Run tests inside builder ──────────────────────────────────────────────────
RUN cd build && ctest --output-on-failure

# ── Runtime stage ─────────────────────────────────────────────────────────────
FROM ubuntu:24.04 AS runtime

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update -qq && \
    apt-get install -y --no-install-recommends \
        libfftw3-single3 portaudio19-dev \
        python3 python3-pip && \
    rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY --from=builder /src/build/neural_echo_tests  ./
COPY --from=builder /src/build/neural_echo_bench   ./
COPY --from=builder /src/ml                        ./ml/
COPY --from=builder /src/models                    ./models/

# Python ML deps for evaluation
RUN pip3 install --no-cache-dir numpy soundfile pesq pystoi

CMD ["./neural_echo_tests"]
