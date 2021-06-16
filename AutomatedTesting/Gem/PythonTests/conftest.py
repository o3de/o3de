# content of conftest.py
import pytest
import os

def pytest_addoption(parser):
    parser.addoption("--tiaf_coverage_dir", action="store", help="Directory to store Gem coverage data")


def pytest_configure(config):
    os.environ["tiaf_coverage_dir"] = config.getoption('tiaf_coverage_dir')