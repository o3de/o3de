"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
"""

#
# Helpful functions to simplify running the Hydra tests
#
import pytest
pytest.importorskip('ly_test_tools')

import os
import ly_test_tools.log.log_monitor
import ly_test_tools.environment.waiter as waiter

import logging
logger = logging.getLogger(__name__)

def launch_test_case(editor, test_case, expected_lines, unexpected_lines):

    timeout=180
    halt_on_unexpected=False
    logger.debug("Running automated test: {}".format(test_case))
    editor.args.extend(['-NullRenderer', "--skipWelcomeScreenDialog", "--autotest_mode", "--runpythontest", test_case])
    print ('editor.args = {}'.format(editor.args))

    with editor.start():
        editorlog_file = os.path.join(editor.workspace.paths.project_log(), 'Editor.log')
        log_monitor = ly_test_tools.log.log_monitor.LogMonitor(editor, editorlog_file)
        logger.debug("Waiting for log file '{}' to be opened by another process.".format(editorlog_file))
        waiter.wait_for(
            lambda: editor.is_alive(),
            timeout,
            exc=("Log file '{}' was never opened by another process.".format(editorlog_file)),
            interval=1)
        log_monitor.monitor_log_for_lines(expected_lines, unexpected_lines, halt_on_unexpected, timeout)

def launch_test_case_with_args(editor, test_case, expected_lines, unexpected_lines, extra_args):

    timeout=180
    halt_on_unexpected=False
    logger.debug("Running automated test: {}".format(test_case))
    editor.args.extend(['-NullRenderer', "--skipWelcomeScreenDialog", "--autotest_mode", "--runpythontest", test_case, '--runpythonargs'])
    editor.args.extend(extra_args)
    print ('editor.args = {}'.format(editor.args))

    with editor.start():
        editorlog_file = os.path.join(editor.workspace.paths.project_log(), 'Editor.log')
        log_monitor = ly_test_tools.log.log_monitor.LogMonitor(editor, editorlog_file)
        logger.debug("Waiting for log file '{}' to be opened by another process.".format(editorlog_file))
        waiter.wait_for(
            lambda: editor.is_alive(),
            timeout,
            exc=("Log file '{}' was never opened by another process.".format(editorlog_file)),
            interval=1)
        log_monitor.monitor_log_for_lines(expected_lines, unexpected_lines, halt_on_unexpected, timeout)
