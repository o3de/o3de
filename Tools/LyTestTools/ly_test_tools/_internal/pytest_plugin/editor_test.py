# Copyright (c) Contributors to the Open 3D Engine Project
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT

"""
Utility for specifying an Editor test, supports seamless parallelization and/or batching of tests.
"""

import pytest
import inspect

def pytest_addoption(parser):
    parser.addoption("--no-editor-batch", action="store_false", help="Don't batch multiple tests in single editor")
    parser.addoption("--no-editor-parallel", action="store_false", help="Don't run multiple editors in parallel")
    parser.addoption("--parallel-editors", type=int, action="store", help="Override the number editors to run at the same time")

# Create a custom custom item collection if the class defines pytest_custom_makeitem function
# This is used for automtically generating test functions with a custom collector
def pytest_pycollect_makeitem(collector, name, obj):
    if inspect.isclass(obj):
        for base in obj.__bases__:
            if hasattr(base, "pytest_custom_makeitem"):
                return base.pytest_custom_makeitem(collector, name, obj)
