"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Utility functions mostly for the editor_test module. They can also be used for assisting Editor tests.
"""
from __future__ import annotations
import logging
import os
import re
import time

import ly_test_tools.environment.process_utils as process_utils
import ly_test_tools.environment.waiter as waiter
from ly_test_tools._internal.exceptions import EditorToolsFrameworkException
from ly_test_tools.o3de.asset_processor import AssetProcessor

logger = logging.getLogger(__name__)


def compile_test_case_name_from_request(request):
    """
    Compile a test case name for consumption by the TIAF python coverage listener gem.
    @param request: The fixture request.
    """
    try:
        test_case_prefix = "::".join(str.split(request.node.nodeid, "::")[:2])
        test_case_name = "::".join([test_case_prefix, request.node.originalname])
        callspec = request.node.callspec.id
        compiled_test_case_name = f"{test_case_name}[{callspec}]"
    except Exception as e:
        logging.warning(f"Error reading test case name for TIAF. {e}")
        compiled_test_case_name = "ERROR"
    return compiled_test_case_name


def kill_all_ly_processes(include_asset_processor: bool = True) -> None:
    """
    Kills all common O3DE processes such as the Editor, Game Launchers, and optionally Asset Processor. Defaults to
    killing the Asset Processor.
    :param include_asset_processor: Boolean flag whether or not to kill the AP
    :return: None
    """
    ly_processes = [
        'Editor', 'Profiler', 'RemoteConsole', 'o3de', 'AutomatedTesting.ServerLauncher', 'MaterialEditor',
        'MaterialCanvas'
    ]
    ap_processes = [
        'AssetProcessor', 'AssetProcessorBatch', 'AssetBuilder'
    ]
    
    if include_asset_processor:
        process_utils.kill_processes_named(ly_processes+ap_processes, ignore_extensions=True)
    else:
        process_utils.kill_processes_named(ly_processes, ignore_extensions=True)


def get_testcase_module_filepath(testcase_module: types.ModuleType) -> str:
    """
    return the full path of the test module using always '.py' extension
    :param testcase_module: The testcase python module being tested
    :return str: The full path to the testcase module
    """
    return os.path.splitext(testcase_module.__file__)[0] + ".py"


def get_module_filename(testcase_module: types.ModuleType) -> str:
    """
    return The filename of the module without path
    Note: This is differs from module.__name__ in the essence of not having the package directory.
    for example: /mylibrary/myfile.py will be "myfile" instead of "mylibrary.myfile"
    :param testcase_module: The testcase python module being tested
    :return str: Filename of the module
    """
    return os.path.splitext(os.path.basename(testcase_module.__file__))[0]


def retrieve_log_path(run_id: int, workspace: ly_test_tools._internal.managers.workspace.AbstractWorkspaceManager) -> str:
    """
    return the log/ project path for this test run.
    :param run_id: editor id that will be used for differentiating paths
    :param workspace: Workspace fixture
    :return str: The full path to the given editor the log/ path
    """
    return os.path.join(workspace.paths.project(), "user", f"log_test_{run_id}")


def atom_tools_log_path(
        run_id: int, workspace: ly_test_tools._internal.managers.workspace.AbstractWorkspaceManager) -> str:
    """
    return the log path for Atom tools test runs
    :param run_id: executable id that will be used for differentiating paths
    :param workspace: Workspace fixture
    :return: str: The full path to the given executable log
    """
    return os.path.join(workspace.paths.project(), "user", "log", f"log_test_{run_id}")


def retrieve_crash_output(run_id: int, workspace: ly_test_tools._internal.managers.workspace.AbstractWorkspaceManager,
                          timeout: float = 10) -> str:
    """
    returns the crash output string for the given test run.
    :param run_id: editor id that will be used for differentiating paths
    :param workspace: Workspace fixture
    :param timeout: Maximum time (seconds) to wait for crash output file to appear
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

    try:
        with open(crash_log) as f:
            crash_info = f.read()
    except Exception as ex:  # Even if the path didn't exist, we are interested on the exact reason why it couldn't be read
        crash_info += f"\n{str(ex)}"
    return crash_info


def cycle_crash_report(run_id: int, workspace: ly_test_tools._internal.managers.workspace.AbstractWorkspaceManager) -> None:
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


def retrieve_editor_log_content(run_id: int,
                                log_name: str,
                                workspace: ly_test_tools._internal.managers.workspace.AbstractWorkspaceManager,
                                timeout: int = 10) -> str:
    """
    Retrieves the contents of the given editor log file.
    :param run_id: editor id that will be used for differentiating paths
    :param log_name: The name of the editor log to retrieve
    :param workspace: Workspace fixture
    :param timeout: Maximum time to wait for the log file to appear
    :return str: The contents of the log
    """
    editor_info = "-- No Editor log available --"
    editor_log = os.path.join(retrieve_log_path(run_id, workspace), log_name)
    try:
        waiter.wait_for(lambda: os.path.exists(editor_log), timeout=timeout)
    except AssertionError:              
        pass

    # Even if the path didn't exist, we are interested on the exact reason why it couldn't be read
    try:
        with open(editor_log) as opened_log:
            editor_info = ""
            for line in opened_log:
                editor_info += f"[{log_name}]  {line}"
    except Exception as ex:
        editor_info = f"-- Error reading {log_name}: {str(ex)} --"
    return editor_info


def retrieve_non_editor_log_content(run_id: int,
                                    log_name: str,
                                    workspace: ly_test_tools._internal.managers.workspace.AbstractWorkspaceManager,
                                    timeout: int = 10) -> str:
    """
    Retrieves the contents of the given path to the executable log file.
    :param run_id: executable id that will be used for differentiating paths
    :param log_name: The name of the executable log being retrieved
    :param workspace: Workspace fixture
    :param timeout: Maximum time to wait for the log file to appear
    :return str: The contents of the log
    """
    atom_tools_log = os.path.join(atom_tools_log_path(run_id, workspace), log_name)
    try:
        waiter.wait_for(lambda: os.path.exists(atom_tools_log), timeout=timeout)
    except AssertionError:
        pass  # Even if the path didn't exist, we are interested on the exact reason why it couldn't be read

    try:
        with open(atom_tools_log) as opened_log:
            editor_info = ""
            for line in opened_log:
                editor_info += f"[{log_name}]  {line}"
    except Exception as ex:
        editor_info = f"-- Error reading {log_name}: {str(ex)} --"
    return editor_info


def retrieve_last_run_test_index_from_output(test_spec_list: list[EditorTest], output: str) -> int:
    """
    Finds out what was the last test that was run by inspecting the input.
    This is used for determining what was the batched test has crashed the editor
    :param test_spec_list: List of tests that were run in this editor
    :param output: Editor output to inspect
    :return: Index in the given test_spec_list of the last test that ran
    """
    index = -1
    find_pos = 0
    for test_spec in test_spec_list:
        find_pos = output.find(test_spec.__name__, find_pos)
        if find_pos == -1:
            index = max(index, 0)  # didn't even find the first test, assume the first one immediately crashed
            return index
        else:
            index += 1
    return index


def save_failed_asset_joblogs(workspace: ly_test_tools._internal.managers.workspace.AbstractWorkspaceManager) -> None:
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
                    workspace.artifact_manager.save_artifact(
                        full_log_path, os.path.join("assets_logs", os.path.basename(full_log_path)))
                except Exception as e:  # Purposefully broad
                    logger.warning(f"Error when saving log at path: {full_log_path}\n{e}")


def compile_test_case_name(request, test_spec):
    """
    Compile a test case name for consumption by the TIAF python coverage listener gem.
    :param request: The fixture request.
    :param test_spec: The test spec for this test case.
    """
    test_case_prefix = "::".join(str.split(str(request.node.nodeid), "::")[:2])
    regex_result = re.search("\<class \'(.*)\'\>", str(test_spec))
    try:
        class_name = str.split(regex_result.group(1), ".")[-1:]
        test_case_name = f"{'::'.join([test_case_prefix, class_name[0]])}"
        callspec = request.node.callspec.id
        compiled_test_case_name = f"{test_case_name}[{callspec}]"
    except Exception as e:
        logging.warning(f"Error reading test case name for TIAF. {e}")
        compiled_test_case_name = "ERROR"
    return compiled_test_case_name


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
    with open(log_path, 'r', encoding='UTF-8', errors='ignore') as opened_asset_log:
        for log_line in opened_asset_log:
            regex_match = re.search(log_regex, log_line)
            if regex_match is not None:
                break
    # If we match any non zero numbers in: n error, n warnings
    if regex_match is None or int(regex_match.group(1)) != 0 or int(regex_match.group(2)) != 0:
        return True
    return False


def prepare_asset_processor(workspace: ly_test_tools._internal.managers.workspace.AbstractWorkspaceManager,
                            collected_test_data: TestData) -> None:
    """
    Prepares the asset processor for the test depending on whether the process is open and if the current test owns it.
    :param workspace: The workspace object in case an AssetProcessor object needs to be created
    :param collected_test_data: The test data from calling collected_test_data()
    :return: None
    """
    try:
        # Start-up an asset processor if we are not already managing one
        if collected_test_data.asset_processor is None:
            if not process_utils.process_exists("AssetProcessor", ignore_extensions=True):
                kill_all_ly_processes(include_asset_processor=True)
                collected_test_data.asset_processor = AssetProcessor(workspace)
                collected_test_data.asset_processor.start()
            else:  # If another AP process already exists, do not kill it as we do not manage it
                kill_all_ly_processes(include_asset_processor=False)
        else:  # Make sure existing asset processor wasn't closed by accident
            collected_test_data.asset_processor.start()
    except Exception as ex:
        collected_test_data.asset_processor = None
        raise EditorToolsFrameworkException from ex
