"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.

Utility for specifying an Editor test, supports seamless parallelization and/or batching of tests.

"""
import pytest

def pytest_addoption(parser):
    parser.addoption("--no-editor-batch", action="store_false", help="Don't batch multiple tests in single editor")
    parser.addoption("--no-editor-parallel", action="store_false", help="Don't run multiple editors in parallel")
    parser.addoption("--parallel-editors", action="store", help="Override the number editors to run at the same time")

def pytest_pycollect_makeitem(collector, name, obj):
    import inspect
    if inspect.isclass(obj):
        for base in obj.__bases__:
            if hasattr(base, "pytest_custom_makeitem"):
                return base.pytest_custom_makeitem(collector, name, obj)
