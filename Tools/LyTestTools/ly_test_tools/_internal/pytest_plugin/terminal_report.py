"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
import os
import re

import ly_test_tools._internal.pytest_plugin.failed_test_rerun_command as rerun
import ly_test_tools.cli.codeowners_hint as hint

UNKNOWN_TEST_RESULT = "Indeterminate test result interpreted as failure, possible cause:"


def _add_commands(terminalreporter, header, test_path, node_ids):
    """
    Add test re-run commands to the TerminalReporter object
    :param terminalreporter: Pytest's TerminalReporter object that contains test result information.
    :param header: Message to write to TerminalReporter before list is added
    :param test_path: File or directory that contains the test(s) that to run
    :param node_ids: List of test node ids, with parametrized values
    """
    terminalreporter.write_line(header)

    if node_ids:
        commands = rerun.build_rerun_commands(test_path, node_ids)
        for command in commands:
            terminalreporter.write_line(command)
        terminalreporter.currentfspath = 1
        terminalreporter.ensure_newline()
    else:
        terminalreporter.write_line("Error: Cannot provide rerun commands because test node id list is empty!")


def _add_ownership(terminalreporter, test_path):
    """
    :param terminalreporter: Pytest's TerminalReporter object that contains test result information
    :param test_path: File or directory that contains the test(s) that were run
    """
    test_path = os.path.abspath(test_path)
    matched_path, owner_aliases, codeowners_path = hint.get_codeowners(test_path)
    if owner_aliases and matched_path and codeowners_path:
        hint._pretty_print_success(terminalreporter.write_line, codeowners_path, matched_path, owner_aliases)
    else:
        hint._pretty_print_failure(terminalreporter.write_line, codeowners_path, matched_path, test_path)


def pytest_terminal_summary(terminalreporter, exitstatus, config):
    """
    Pytest's hook for terminal reporting. This hook is invoked at the end of the test session.

    :param terminalreporter: Pytest's TerminalReporter object that contains test result information.
    :param config: The pytest.Config object
    """
    # Add to the TerminalReport a section for failed test re-running
    failures = terminalreporter.stats.get('failed', [])
    errors = terminalreporter.stats.get('error', [])
    failure_count = len(failures)
    error_count = len(errors)

    if failure_count or error_count:
        file_or_dir_option = terminalreporter.config.getoption('file_or_dir', default=[])
        test_path = ''
        if file_or_dir_option:
            test_path = file_or_dir_option[0]

        terminalreporter.currentfspath = 1
        terminalreporter.ensure_newline()
        terminalreporter.section("Test failure and error troubleshooting")

        if failure_count:
            nodeids = [os.path.basename(report.nodeid) for report in failures]

            _add_commands(
                terminalreporter,
                "Use the following commands to re-run each test that failed locally\n"
                "(NOTE: The 'PYTHON' or 'PYTHONPATH' environment variables need values for accurate commands):\n",
                test_path, nodeids)

            # Check for unknown test failures
            have_printed_header = False
            test_name_pattern = r"::.*::([^\[]*)"
            for report in failures:
                for section in report.sections:
                    if UNKNOWN_TEST_RESULT in str(section):
                        if not have_printed_header:
                            terminalreporter.write_line("\nThe following tests had an unknown result:\n")
                            have_printed_header = True
                        terminalreporter.write_line(re.search(test_name_pattern, report.nodeid).group(1))
                        break

        if error_count:
            nodeids = [os.path.basename(report.nodeid) for report in errors]

            _add_commands(
                terminalreporter,
                "Use the following commands to re-run each test that had errors locally\n"
                "(NOTE: The 'PYTHON' or 'PYTHONPATH' environment variables need values for accurate commands): ",
                test_path, nodeids)

        _add_ownership(terminalreporter, test_path)
