"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
"""

import logging
import os
import tempfile
import ly_test_tools.log.log_monitor
import ly_test_tools.environment.process_utils as process_utils
import ly_test_tools.environment.waiter as waiter

logger = logging.getLogger(__name__)


def teardown_editor(editor):
    """
    :param editor: Configured editor object
    :return:
    """
    process_utils.kill_processes_named('AssetProcessor.exe')
    logger.debug('Ensuring Editor is stopped')
    editor.ensure_stopped()


def launch_and_validate_results(request, test_directory, editor, editor_script, expected_lines, unexpected_lines=[],
                                halt_on_unexpected=False, auto_test_mode=True, run_python="--runpythontest", cfg_args=[], timeout=60, log_creation_max_wait=60):
    """
    Creates a temporary config file for Hydra execution, runs the Editor with the specified script, and monitors for
    expected log lines.
    :param request: Special fixture providing information of the requesting test function.
    :param test_directory: Path to test directory that editor_script lives in.
    :param editor: Configured editor object to run test against.
    :param editor_script: Name of script that will execute in the Editor.
    :param expected_lines: Expected lines to search log for.
    :param unexpected_lines: Unexpected lines to search log for. Defaults to none.
    :param halt_on_unexpected: Halts test if unexpected lines are found. Defaults to False.
    :param auto_test_mode: Defaults to True. Runs the test in auto_test_mode.
    :param run_python: Defaults to "--runpythontest", other option is "--runpython".
    :param cfg_args: Additional arguments for CFG, such as LevelName.
    :param timeout: Length of time for test to run. Default is 60.
    :param log_creation_max_wait: Length of time for waiting to find the log file. Default is 60.
    """
    test_case = os.path.join(test_directory, editor_script)
    request.addfinalizer(lambda: teardown_editor(editor))
    logger.debug("Running automated test: {}".format(editor_script))

    editor.args.extend(["--skipWelcomeScreenDialog"])
    if auto_test_mode: editor.args.extend(["--autotest_mode"])
    editor.args.extend([run_python, test_case, "--runpythonargs", cfg_args])

    with editor.start():

        editorlog_file = os.path.join(editor.workspace.paths.project_log(), 'Editor.log')
        log_monitor = ly_test_tools.log.log_monitor.LogMonitor(launcher=editor, log_file_path=editorlog_file, log_creation_max_wait_time=log_creation_max_wait)
        log_monitor.monitor_log_for_lines(expected_lines=expected_lines, unexpected_lines=unexpected_lines,
                                          halt_on_unexpected=halt_on_unexpected, timeout=timeout)


def remove_files(artifact_path, suffix):
    """
    Removes files with the specified suffix from the specified path
    :param artifact_path: Path to search for files
    :param suffix: File extension to remove
    """
    if not os.path.isdir(artifact_path):
        return

    for file_name in os.listdir(artifact_path):
        if file_name.endswith(suffix):
            os.remove(os.path.join(artifact_path, file_name))
