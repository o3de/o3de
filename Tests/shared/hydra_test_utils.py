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
import test_tools.shared.log_monitor
import test_tools.launchers.phase

logger = logging.getLogger(__name__)


def prepare_cfg_file(editor_python_script_name, args=[]):
    """
    Create a temporary .cfg file containing the python script and args to pass to the Editor.
    Ex: If you pass in 'create_level.py', ['LevelName'], you'll get a cfg file with this:
    pyRunFile create_level.py LevelName
    :param editor_python_script_name: Name of script that will execute in the Editor.
    :param args: Additional arguments for CFG, such as LevelName.
    :return Config file for Hydra execution
    """
    cfg_contents = '-- Auto-generated cfg file\n'
    cfg_contents += 'pyRunFile ' + editor_python_script_name
    for arg in args:
        cfg_contents += ' ' + arg
    cfg_contents += '\n'

    logger.debug("Preparing a cfg file with the following contents:\n{}".format(cfg_contents))
    f = tempfile.NamedTemporaryFile(mode='w+', suffix='.cfg', delete=False)
    cfg_filename = f.name
    f.write(cfg_contents)
    f.close()
    logger.debug("Cfg file name: {}".format(cfg_filename))
    return cfg_filename


def cleanup_cfg_file(cfg_filename):
    """
    Removes the temporary cfg file created in prepare_cfg_file.
    :param cfg_filename: Config file for Hydra execution to delete
    """
    logger.debug('Cleaning up the generated cfg file')
    if os.path.exists(cfg_filename):
        os.remove(cfg_filename)


def launch_and_validate_results(test_directory, editor, editor_script, editor_timeout, expected_lines, cfg_args=[]):
    """
    Creates a temporary config file for Hydra execution, runs the Editor with the specified script, and monitors for
    expected log lines.
    :param test_directory: Path to test directory that editor_script lives in.
    :param editor: Configured editor object to run test against.
    :param editor_script: Name of script that will execute in the Editor.
    :param editor_timeout: Timeout for editor run.
    :param expected_lines: Expected lines to search log for.
    :param cfg_args: Additional arguments for CFG, such as LevelName.
    """
    cfg_file_name = prepare_cfg_file(os.path.join(test_directory, editor_script), cfg_args)

    logger.debug("Running automated test: {}".format(editor_script))

    editor.deploy()
    editor.launch(["--skipWelcomeScreenDialog", "--autotest_mode", "--exec", cfg_file_name])

    editorlog_file = os.path.join(editor.workspace.release.paths.project_log(), 'Editor.log')

    test_tools.shared.log_monitor.monitor_for_expected_lines(editor, editorlog_file, expected_lines)

    # Rely on the test script to quit after running
    editor.run(test_tools.launchers.phase.WaitForLauncherToQuit(editor, editor_timeout))
