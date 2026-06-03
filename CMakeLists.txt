cmake_minimum_required(VERSION 3.27)
project(NeuralEcho VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ── Options ────────────────────────────────────────────────────────────────────
option(NEURAL_ECHO_USE_IPP     "Enable Intel IPP acceleration"  OFF)
option(NEURAL_ECHO_BUILD_TESTS "Build unit tests"               ON)
option(NEURAL_ECHO_BUILD_BENCH "Build benchmark suite"          ON)
option(NEURAL_ECHO_BUILD_QT   "Build Qt 6 Dashboard"            ON)
option(NEURAL_ECHO_USE_NEURAL  "Link Rust neural runtime"        ON)

# ── Dependencies (vcpkg) ───────────────────────────────────────────────────────
find_package(FFTW3f        CONFIG REQUIRED)
find_package(Eigen3        CONFIG REQUIRED)
find_package(xsimd         CONFIG REQUIRED)
find_package(spdlog        CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(tomlplusplus  CONFIG REQUIRED)
find_package(portaudio     CONFIG REQUIRED)

if(NEURAL_ECHO_BUILD_TESTS)
    find_package(GTest CONFIG REQUIRED)
endif()
if(NEURAL_ECHO_BUILD_BENCH)
    find_package(benchmark CONFIG REQUIRED)
endif()
if(NEURAL_ECHO_BUILD_QT)
    find_package(Qt6 REQUIRED COMPONENTS
        Core Gui Quick Charts Multimedia OpenGL Concurrent Widgets)
endif()

# ── Rust neural runtime (optional) ────────────────────────────────────────────
set(RUST_TARGET_DIR ${CMAKE_BINARY_DIR}/rust_target)
set(RUST_LIB_PATH   ${RUST_TARGET_DIR}/release/libneural_echo_runtime.a)
set(RUST_FFI_HEADER ${CMAKE_SOURCE_DIR}/rust/neural_echo_ffi/include/neural_echo_ffi.h)

if(NEURAL_ECHO_USE_NEURAL)
    add_custom_command(
        OUTPUT  ${RUST_LIB_PATH} ${RUST_FFI_HEADER}
        COMMAND cargo build --release --target-dir ${RUST_TARGET_DIR}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/rust
        COMMENT "Building Rust neural runtime"
        VERBATIM
    )
    add_custom_target(rust_runtime_build ALL DEPENDS ${RUST_LIB_PATH})

    add_library(neural_runtime STATIC IMPORTED GLOBAL)
    add_dependencies(neural_runtime rust_runtime_build)
    set_target_properties(neural_runtime PROPERTIES
        IMPORTED_LOCATION         ${RUST_LIB_PATH}
        INTERFACE_INCLUDE_DIRECTORIES
            ${CMAKE_SOURCE_DIR}/rust/neural_echo_ffi/include
    )
endif()

# ── DSP static library ─────────────────────────────────────────────────────────
add_library(neural_echo_dsp STATIC
    dsp/fft/fft_engine.cpp
    dsp/nlms/nlms_filter.cpp
    dsp/dtd/double_talk_detector.cpp
    dsp/ree/residual_echo_estimator.cpp
    dsp/agc/agc.cpp
    dsp/vad/vad.cpp
    dsp/metrics/metrics_collector.cpp
    dsp/neural_bridge.cpp
    dsp/aec_pipeline.cpp
)

target_link_libraries(neural_echo_dsp PUBLIC
    FFTW3::fftw3f
    Eigen3::Eigen
    xsimd::xsimd
    spdlog::spdlog
    nlohmann_json::nlohmann_json
    tomlplusplus::tomlplusplus
)

if(NEURAL_ECHO_USE_NEURAL)
    target_link_libraries(neural_echo_dsp PUBLIC neural_runtime)
    target_compile_definitions(neural_echo_dsp PUBLIC NE_NEURAL_ENABLED=1)
endif()

# Platform link flags for Rust static lib
if(UNIX AND NOT APPLE)
    target_link_libraries(neural_echo_dsp PUBLIC dl pthread)
elseif(APPLE)
    target_link_libraries(neural_echo_dsp PUBLIC "-framework Security" "-framework CoreFoundation")
endif()

target_include_directories(neural_echo_dsp PUBLIC
    ${CMAKE_SOURCE_DIR}/dsp
)

# ── Qt Dashboard ───────────────────────────────────────────────────────────────
if(NEURAL_ECHO_BUILD_QT)
    qt6_add_executable(NeuralEchoDashboard
        apps/qt_dashboard/main.cpp
        apps/qt_dashboard/app_controller.cpp
        apps/qt_dashboard/audio_device_manager.cpp
    )

    qt6_add_qml_module(NeuralEchoDashboard
        URI NeuralEcho
        VERSION 1.0
        QML_FILES
            apps/qt_dashboard/qml/Main.qml
            apps/qt_dashboard/qml/DashboardPage.qml
            apps/qt_dashboard/qml/SpectrogramPage.qml
            apps/qt_dashboard/qml/MetricsPage.qml
            apps/qt_dashboard/qml/ExperimentsPage.qml
            apps/qt_dashboard/qml/ModelInspectorPage.qml
            apps/qt_dashboard/qml/components/WaveformPlot.qml
            apps/qt_dashboard/qml/components/MetricCard.qml
            apps/qt_dashboard/qml/components/SidebarNav.qml
            apps/qt_dashboard/qml/components/StatusBar.qml
            apps/qt_dashboard/qml/components/ThemeConstants.qml
            apps/qt_dashboard/qml/components/SpectrumCanvas.qml
    )

    target_link_libraries(NeuralEchoDashboard PRIVATE
        neural_echo_dsp
        portaudio
        Qt6::Core Qt6::Gui Qt6::Quick Qt6::Charts
        Qt6::Multimedia Qt6::OpenGL Qt6::Concurrent Qt6::Widgets
    )

    target_include_directories(NeuralEchoDashboard PRIVATE
        apps/qt_dashboard
    )
endif()

# ── Unit tests ─────────────────────────────────────────────────────────────────
if(NEURAL_ECHO_BUILD_TESTS)
    enable_testing()
    add_executable(neural_echo_tests
        tests/test_fft_engine.cpp
        tests/test_nlms_filter.cpp
        tests/test_dtd.cpp
        tests/test_ree.cpp
        tests/test_aec_pipeline.cpp
    )
    target_link_libraries(neural_echo_tests
        neural_echo_dsp
        GTest::gtest_main
    )
    include(GoogleTest)
    gtest_discover_tests(neural_echo_tests)
endif()

# ── Benchmarks ─────────────────────────────────────────────────────────────────
if(NEURAL_ECHO_BUILD_BENCH)
    add_executable(neural_echo_bench
        benchmarks/bench_fft.cpp
        benchmarks/bench_nlms.cpp
        benchmarks/bench_pipeline.cpp
    )
    target_link_libraries(neural_echo_bench
        neural_echo_dsp
        benchmark::benchmark_main
    )
endif()
