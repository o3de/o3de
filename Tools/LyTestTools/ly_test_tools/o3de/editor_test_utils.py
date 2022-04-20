"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Utility functions mostly for the editor_test module. They can also be used for assisting Editor tests.
"""
from __future__ import annotations
import os
import time
import logging
import re

import ly_test_tools.environment.process_utils as process_utils
import ly_test_tools.environment.waiter as waiter

logger = logging.getLogger(__name__)


def kill_all_ly_processes(include_asset_processor: bool = True) -> None:
    """
    Kills all common O3DE processes such as the Editor, Game Launchers, and optionally Asset Processor. Defaults to
    killing the Asset Processor.
    :param include_asset_processor: Boolean flag whether or not to kill the AP
    :return: None
    """
    LY_PROCESSES = [
        'Editor', 'Profiler', 'RemoteConsole', 'o3de', 'AutomatedTesting.ServerLauncher'
    ]
    AP_PROCESSES = [
        'AssetProcessor', 'AssetProcessorBatch', 'AssetBuilder'
    ]
    
    if include_asset_processor:
        process_utils.kill_processes_named(LY_PROCESSES+AP_PROCESSES, ignore_extensions=True)
    else:
        process_utils.kill_processes_named(LY_PROCESSES, ignore_extensions=True)


def get_testcase_module_filepath(testcase_module: Module) -> str:
    """
    return the full path of the test module using always '.py' extension
    :param testcase_module: The testcase python module being tested
    :return str: The full path to the testcase module
    """
    return os.path.splitext(testcase_module.__file__)[0] + ".py"


def get_module_filename(testcase_module: Module):
    """
    return The filename of the module without path
    Note: This is differs from module.__name__ in the essence of not having the package directory.
    for example: /mylibrary/myfile.py will be "myfile" instead of "mylibrary.myfile"
    :param testcase_module: The testcase python module being tested
    :return str: Filename of the module
    """
    return os.path.splitext(os.path.basename(testcase_module.__file__))[0]


def retrieve_log_path(run_id: int, workspace: AbstractWorkspaceManager) -> str:
    """
    return the log/ project path for this test run.
    :param run_id: editor id that will be used for differentiating paths
    :param workspace: Workspace fixture
    :return str: The full path to the given editor the log/ path
    """
    return os.path.join(workspace.paths.project(), "user", f"log_test_{run_id}")


def retrieve_crash_output(run_id: int, workspace: AbstractWorkspaceManager, timeout: float = 10) -> str:
    """
    returns the crash output string for the given test run.
    :param run_id: editor id that will be used for differentiating paths
    :param workspace: Workspace fixture
    :timeout: Maximum time (seconds) to wait for crash output file to appear
    :return str: The contents of the editor crash file (error.log)
    """
    crash_info = "-- No crash log available --"
    # Grab the file name of the crash log which can be different depending on platform
    crash_file_name = os.path.basename(workspace.paths.crash_log())
    crash_log = os.path.join(retrieve_log_path(run_id, workspace), crash_file_name)
    try:
        waiter.wait_for(lambda: os.path.exists(crash_log), timeout=timeout)
    except AssertionError:                    
        pass
        
    # Even if the path didn't exist, we are interested on the exact reason why it couldn't be read
    try:
        with open(crash_log) as f:
            crash_info = f.read()
    except Exception as ex:
        crash_info += f"\n{str(ex)}"
    return crash_info


def cycle_crash_report(run_id: int, workspace: AbstractWorkspaceManager) -> None:
    """
    Attempts to rename error.log and error.dmp(crash files) into new names with the timestamp on it.
    :param run_id: editor id that will be used for differentiating paths
    :param workspace: Workspace fixture
    """
    log_path = retrieve_log_path(run_id, workspace)
    files_to_cycle = ['crash.log', 'error.log', 'error.dmp']
    for filename in files_to_cycle:
        filepath = os.path.join(log_path, filename)
        name, ext = os.path.splitext(filename)
        if os.path.exists(filepath):
            try:
                modTimesinceEpoc = os.path.getmtime(filepath)
                modStr = time.strftime('%Y_%m_%d_%H_%M_%S', time.localtime(modTimesinceEpoc))
                new_filepath = os.path.join(log_path, f'{name}_{modStr}{ext}')
                os.rename(filepath, new_filepath)
            except Exception as ex:
                logger.warning(f"Couldn't cycle file {filepath}. Error: {str(ex)}")


def retrieve_editor_log_content(run_id: int, log_name: str, workspace: AbstractWorkspaceManager, timeout: int = 10) -> str:
    """
    Retrieves the contents of the given editor log file.
    :param run_id: editor id that will be used for differentiating paths
    :log_name: The name of the editor log to retrieve
    :param workspace: Workspace fixture
    :timeout: Maximum time to wait for the log file to appear
    :return str: The contents of the log
    """
    editor_info = "-- No editor log available --"
    editor_log = os.path.join(retrieve_log_path(run_id, workspace), log_name)
    try:
        waiter.wait_for(lambda: os.path.exists(editor_log), timeout=timeout)
    except AssertionError:              
        pass
        
    # Even if the path didn't exist, we are interested on the exact reason why it couldn't be read
    try:
        with open(editor_log) as f:
            editor_info = ""
            for line in f:
                editor_info += f"[{log_name}]  {line}"
    except Exception as ex:
        editor_info = f"-- Error reading {log_name}: {str(ex)} --"
    return editor_info


def retrieve_last_run_test_index_from_output(test_spec_list: list[EditorTestBase], output: str) -> int:
    """
    Finds out what was the last test that was run by inspecting the input.
    This is used for determining what was the batched test has crashed the editor
    :param test_spec_list: List of tests that were run in this editor
    :output: Editor output to inspect
    :return: Index in the given test_spec_list of the last test that ran
    """
    index = -1
    find_pos = 0
    for test_spec in test_spec_list:
        find_pos = output.find(test_spec.__name__, find_pos)
        if find_pos == -1:
            index = max(index, 0) # <- if we didn't even find the first test, assume its been the first one that crashed
            return index
        else:
            index += 1
    return index


def save_failed_asset_joblogs(workspace: AbstractWorkspace) -> None:
    """
    Checks all asset logs in the JobLogs directory to see if the asset has any warnings or errors. If so, the asset is
    saved via ArtifactManager.

    :param workspace: The AbstractWorkspace to access the JobLogs path
    :return: None
    """
    for walk_tuple in os.walk(workspace.paths.ap_job_logs()):
        for log_file in walk_tuple[2]:
            full_log_path = os.path.join(walk_tuple[0], log_file)
            # Only save asset logs that contain errors or warnings
            if _check_log_errors_warnings(full_log_path):
                try:
                    workspace.artifact_manager.save_artifact(full_log_path)
                except Exception as e:  # Purposefully broad
                    logger.warning(f"Error when saving log at path: {full_log_path}\n{e}")


def _check_log_errors_warnings(log_path: str) -> bool:
    """
    Checks to see if the asset log contains any errors or warnings. Also returns True is no regex is found because
    something probably went wrong.
    Example log lines: ~~1643759303647~~1~~00000000000009E0~~AssetBuilder~~S: 0 errors, 1 warnings

    :param log_path: The full path to the asset log file to read
    :return: True if the regex finds an error or warning, else False
    """
    regex_match = None
    if not os.path.exists(log_path):
        logger.warning(f"Could not find path {log_path} during asset log collection.")
        return False
    log_regex = "(\\d+) errors, (\\d+) warnings"
    with open(log_path, 'r') as opened_asset_log:
        for log_line in opened_asset_log:
            regex_match = re.search(log_regex, log_line)
            if regex_match is not None:
                break
    # If we match any non zero numbers in: n error, n warnings
    if regex_match is None or (int)(regex_match.group(1)) != 0 or (int)(regex_match.group(2)) != 0:
        return True
    return False
