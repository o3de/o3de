"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

PyTest plugin for collecting an EditorTest, and setting commandline parameters. Intended to only be called by PyTest.
"""
from __future__ import annotations
import pytest
import inspect
import ly_test_tools.o3de.editor_test

__test__ = False


def pytest_addoption(parser: argparse.ArgumentParser) -> None:
    """
    Options when running editor tests in batches or parallel.
    :param parser: The ArgumentParser object
    :return: None
    """
    parser.addoption("--no-editor-batch", action="store_true", help="Disable batching multiple tests within each single editor")
    parser.addoption("--no-editor-parallel", action="store_true", help="Disable running multiple editors in parallel")
    parser.addoption("--editors-parallel", type=int, action="store",
                     help="Override the number editors to run at the same time. Tests can override also this value, "
                          f"which has a higher precedence than this option. Default value is: "
                          f"{ly_test_tools.o3de.editor_test.EditorTestSuite.get_number_parallel_editors()}")


def pytest_pycollect_makeitem(collector: PyCollector, name: str, obj: object) -> PyCollector:
    """
    Create a custom custom item collection if the class defines pytest_custom_makeitem function. This is used for
    automatically generating test functions with a custom collector.
    :param collector: The Pytest collector
    :param name: Name of the collector
    :param obj: The custom collector, normally an EditorTestSuite.EditorTestClass object
    :return: Returns the custom collector
    """
    if inspect.isclass(obj):
        for base in obj.__bases__:
            if hasattr(base, "pytest_custom_makeitem"):
                return base.pytest_custom_makeitem(collector, name, obj)


@pytest.hookimpl(hookwrapper=True)
def pytest_collection_modifyitems(session: Session, items: list[EditorTest], config: Config) -> None:
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

