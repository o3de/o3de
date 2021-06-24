"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
"""
import os

import ly_test_tools.environment.process_utils as process_utils
import ly_test_tools.environment.waiter as waiter

class EditorUtils():

    @staticmethod
    def kill_all_ly_processes(include_asset_processor=True):
        LY_PROCESSES = [
            'Editor', 'Profiler', 'RemoteConsole',
        ]
        AP_PROCESSES = [
            'AssetProcessor', 'AssetProcessorBatch', 'AssetBuilder', 'CrySCompileServer',
            'rc'  # Resource Compiler
        ]
        
        if include_asset_processor:
            process_utils.kill_processes_named(LY_PROCESSES+AP_PROCESSES, ignore_extensions=True)
        else:
            process_utils.kill_processes_named(LY_PROCESSES, ignore_extensions=True)

    @staticmethod
    def get_testcase_module_filepath(testcase_module):
        # type: (Module) -> str
        """
        return the full path of the test module
        :param testcase_module: The testcase python module being tested
        :return str: The full path to the testcase module
        """
        return os.path.splitext(testcase_module.__file__)[0] + ".py"

    @staticmethod
    def get_testcase_module_name(testcase_module):
        # type: (Module) -> str
        """
        return the full path of the test module
        :param testcase_module: The testcase python module being tested
        :return str: The full path to the testcase module
        """
        return os.path.basename(os.path.splitext(testcase_module.__file__)[0])

    @staticmethod
    def retrieve_user_path(run_id : int, workspace):
        return os.path.join(workspace.paths.project(), f"user_test_{run_id}")
    
    @staticmethod
    def retrieve_log_path(run_id : int, workspace):
        return os.path.join(EditorUtils.retrieve_user_path(run_id, workspace), "log")
    
    @staticmethod
    def retrieve_crash_output(run_id : int, workspace, timeout):
        crash_info = "-- No crash log available --"
        crash_log = os.path.join(EditorUtils.retrieve_log_path(run_id, workspace), 'error.log')
        try:
            waiter.wait_for(lambda: os.path.exists(crash_log), timeout=timeout)
        except AssertionError:                    
            pass
            
        try:
            with open(crash_log) as f:
                crash_info = f.read()
        except Exception as ex:
            crash_info += f"\n{str(ex)}"
        return crash_info

    @staticmethod
    def delete_crash_report(run_id : int, workspace):
        crash_log = os.path.join(EditorUtils.retrieve_log_path(run_id, workspace), 'error.log')
        if os.path.exists(crash_log):
            os.remove(crash_log)

    @staticmethod
    def retrieve_editor_log_content(run_id : int, log_name : str, workspace, timeout=10):
        editor_info = "-- No editor log available --"
        editor_log = os.path.join(EditorUtils.retrieve_log_path(run_id, workspace), log_name)
        try:
            waiter.wait_for(lambda: os.path.exists(editor_log), timeout=timeout)
        except AssertionError:              
            pass
            
        try:
            with open(editor_log) as f:
                editor_info = ""
                for line in f:
                    editor_info += f"[editor.log]  {line}"
        except Exception as ex:
            editor_info = f"-- Error reading editor.log: {str(ex)} --"
        return editor_info

    @staticmethod
    def retrieve_last_run_test_index_from_output(test_spec_list, output : str):
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
