"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Adds a hook for the @pytest.mark.test_case_id decorator which allows users to mark
tests with test case IDs that show up on pytest .xml reports.

Additionally, users can use the "--test-case-ids" CLI argument to only run the
comma-separated test case IDs listed after the arg.
"""

import logging
import pytest
import six

ID_MARKER = "test_case_id"

log = logging.getLogger(__name__)


class TestCaseIDException(Exception):
    """Raised when "--test-case-ids" CLI arg is invalid."""
    pass


def pytest_configure(config):
    """
    Pytest configuration for @pytest.mark.test_case_id markers.
    """
    config.addinivalue_line(
        'markers',
        'test_case_id: Add test case ID to test.',
    )


def pytest_addoption(parser):
    """
    Will cause only the comma-separated list of test case IDs
    listed after '--test-case-ids' to be run.
    :param parser: pytest-provided fixture for argparse.ArgumentParser
    """
    parser.addoption("-I", "--test-case-ids", nargs='?',
                     help="Only run tests marked with the specified ID(s).")


def _pytest_runtest_makereport_imp(report, item):
    try:
        test_case_id_marker = item.get_marker(ID_MARKER)
    except AttributeError:  # use get_closest_marker() instead
        test_case_id_marker = item.get_closest_marker(ID_MARKER)

    if report.when == 'call' and test_case_id_marker is not None:
        test_case_ids = _parse_test_case_ids(test_case_id_marker.args)
        try:
            xml_report = getattr(item.config, '_xml')
        except AttributeError:
            log.warning('No .xml report found in test, skipping pytest reporting hooks.')
            return

        for test_case_id in test_case_ids:
            report_node = xml_report.node_reporter(report.nodeid)
            report_node.add_property('test_case_id', test_case_id)


@pytest.hookimpl(hookwrapper=True, trylast=True)
def pytest_runtest_makereport(item, call):
    """
    Modify the pytest jUnitXML report to add test case IDs.
    Tests require the @pytest.mark.test_case_id decorator
    in order to have the test case ID appear on the report.
    :param item: Test in the current pytest runner session.
    :param call: Call in the current pytest runner session.
    """
    outcome = yield
    report = outcome.get_result()
    _pytest_runtest_makereport_imp(report, item)


def pytest_collection_modifyitems(items, config):
    """
    Reduces test collection to tests with the 'test_case_id'
    specified by the user using the --test-case-ids CLI arg.
    :param items: All tests within the pytest runner session.
    :param config: Call config for pytest runner session.
    """
    id_filtering_enabled = config.option.test_case_ids
    selected_items = set()
    deselected_items = set()

    if id_filtering_enabled:
        filtered_test_case_ids = _parse_test_case_ids(
            id_filtering_enabled.split(','))
        for item in items:
            try:
                test_case_id_marker = item.get_marker(ID_MARKER)
            except AttributeError:
                log.debug('item.get_marker() call failed, using item.get_closest_marker() instead.')
                test_case_id_marker = item.get_closest_marker(ID_MARKER)
            test_case_ids = _parse_test_case_ids(test_case_id_marker.args)

            if _any_exist_in(test_case_ids, filtered_test_case_ids):
                selected_items.add(item)
            else:
                deselected_items.add(item)

        config.hook.pytest_deselected(items=deselected_items)
        items[:] = selected_items


def _any_exist_in(select_from, find_in):
    """
    :param select_from: iterable keys to find
    :param find_in: iterable to search in
    :return: True if any item in the first iterable exists in the second
    """
    for target in select_from:
        if target in find_in:
            return True
    return False


def _parse_test_case_ids(test_case_ids):
    """
    Return a set representing unique test case ID values
    :param test_case_ids: list or tuple containing test case ID values (strings or ints)
    :return: set of unique values
    """
    parsed_ids = set()

    for test_case_id in test_case_ids:
        if type(test_case_id) == int:
            parsed_ids.add(test_case_id)
        else:
            try:  # dedupe strings which are identical to integers
                target_id = int(test_case_id)
            except ValueError as err:
                log.debug(err)
                if type(test_case_id) == str and len(test_case_id.strip()) > 0:
                    target_id = test_case_id  # valid string
                else:
                    problem = TestCaseIDException('Invalid test_case_id detected: "{}", test_case_id must be of type '
                                                  'str or int.'.format(test_case_id))
                    six.raise_from(problem, err)
            parsed_ids.add(target_id)

    return parsed_ids
