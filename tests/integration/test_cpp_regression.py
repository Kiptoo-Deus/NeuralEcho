"""
Integration regression test — runs the compiled neural_echo_tests binary
and asserts that the AEC pipeline ERLE regression gate passes.

Usage:
    pytest tests/integration/test_cpp_regression.py -v \
        --build-dir build/

Can also be run by CI after `cmake --build`.
"""

import subprocess
import sys
import pathlib
import pytest


def find_test_binary(build_dir: pathlib.Path) -> pathlib.Path:
    candidates = [
        build_dir / "neural_echo_tests",
        build_dir / "Release" / "neural_echo_tests.exe",
        build_dir / "neural_echo_tests.exe",
    ]
    for p in candidates:
        if p.exists():
            return p
    return None


def pytest_addoption(parser):
    parser.addoption("--build-dir", default="build",
                     help="CMake build directory")


@pytest.fixture(scope="session")
def build_dir(request):
    return pathlib.Path(request.config.getoption("--build-dir")).resolve()


class TestCppBinary:

    def test_binary_exists(self, build_dir):
        binary = find_test_binary(build_dir)
        if binary is None:
            pytest.skip(f"neural_echo_tests binary not found in {build_dir}. "
                        "Run cmake --build first.")

    def test_all_gtest_pass(self, build_dir):
        binary = find_test_binary(build_dir)
        if binary is None:
            pytest.skip("neural_echo_tests not built")

        result = subprocess.run(
            [str(binary), "--gtest_color=no"],
            capture_output=True, text=True, timeout=120
        )
        print(result.stdout[-3000:] if len(result.stdout) > 3000 else result.stdout)
        if result.returncode != 0:
            print("STDERR:", result.stderr[-1000:])
        assert result.returncode == 0, \
            f"GTest suite failed (exit {result.returncode})"

    def test_erle_regression_passes(self, build_dir):
        """Specifically check the ERLE regression test passes."""
        binary = find_test_binary(build_dir)
        if binary is None:
            pytest.skip("neural_echo_tests not built")

        result = subprocess.run(
            [str(binary), "--gtest_filter=AECPipelineTest.ERLERegressionAbove10dB",
             "--gtest_color=no"],
            capture_output=True, text=True, timeout=60
        )
        assert result.returncode == 0, \
            f"ERLE regression test failed:\n{result.stdout}\n{result.stderr}"
