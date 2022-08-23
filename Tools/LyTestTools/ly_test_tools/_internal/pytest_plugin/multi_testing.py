"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Utility for supporting seamless parallel and/or batched sets of tests.
These tools are intended to be called only by the pytest framework and not invoked directly.
"""

from __future__ import annotations

__test__ = False  # Avoid pytest collection & warnings since this module is for test functions, but not a test itself.

import inspect
import ly_test_tools.o3de.multi_test_framework

import pytest


def pytest_addoption(parser: argparse.ArgumentParser) -> None:
    """
    Options when running multiple tests in batches or parallel.
    :param parser: The ArgumentParser object
    :return: None
    """
    parser.addoption("--no-test-batch", action="store_true", help="Don't batch multiple tests in single instance")
    parser.addoption("--no-test-parallel", action="store_true", help="Don't run multiple instances in parallel")
    parser.addoption("--parallel-executables", type=int, action="store",
                     help="Override the number of program executables to run at the same time. Default value is: "
                     f"{ly_test_tools.o3de.multi_test_framework.MultiTestSuite.get_number_parallel_executables()}")


def pytest_pycollect_makeitem(collector: _pytest.python.Module, name: str, obj: object) -> _pytest.python.Module:
    """
    Create a custom item collection if the class defines a pytest_multitest_makeitem method. This is used for
    automatically generating test functions with a custom collector.
    Classes that inherit the MultiTestSuite class require a "pytest_multitest_makeitem" method to collect tests.
    :param collector: The Pytest collector
    :param name: Name of the collector
    :param obj: The custom collector, normally a test class object inside the MultiTestSuite class
    :return: Returns the custom collector
    """
    if inspect.isclass(obj):
        for base in obj.__bases__:
            if hasattr(base, "pytest_multitest_makeitem"):
                return base.pytest_multitest_makeitem(collector, name, obj)


@pytest.hookimpl(hookwrapper=True)
def pytest_collection_modifyitems(session: Session, items: list[AbstractTestBase], config: Config) -> None:
    """
    Add custom modification of items. This is used for adding the runners into the item list.
    :param session: The Pytest Session
    :param items: The test case functions
    :param config: The Pytest Config object
    :return: None
    """
    all_classes = set()
    for item in items:
        all_classes.add(item.instance.__class__)

    yield

    for cls in all_classes:
        if hasattr(cls, "pytest_custom_modify_items"):
            cls.pytest_custom_modify_items(session, items, config)
