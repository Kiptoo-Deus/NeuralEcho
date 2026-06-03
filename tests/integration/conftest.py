# conftest.py — shared pytest fixtures for integration tests
import sys
import pathlib

# Ensure ml/ is importable from integration tests
sys.path.insert(0, str(pathlib.Path(__file__).parents[2]))
