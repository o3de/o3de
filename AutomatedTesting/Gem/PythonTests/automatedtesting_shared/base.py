"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

"""

import os
import logging
import sys
import pytest
import time

import ly_test_tools.environment.file_system as file_system
import ly_test_tools.environment.process_utils as process_utils
import ly_test_tools.environment.waiter as waiter

from ly_test_tools.o3de.asset_processor import AssetProcessor
from ly_test_tools.launchers.exceptions import WaitTimeoutError
from ly_test_tools.log.log_monitor import LogMonitor, LogMonitorException

class TestRunError():
    def __init__(self, title, content):
        self.title = title
        self.content = content

class TestAutomationBase:
    MAX_TIMEOUT = 180 # 3 minutes max for a test to run
    WAIT_FOR_CRASH_LOG = 20 # Seconds for waiting for a crash log
    TEST_FAIL_RETCODE = 0xF # Return code for test failure 
        
    test_times = {}
    asset_processor = None 
        
    def setup_class(cls):
        cls.test_times = {}
        cls.editor_times = {}
        cls.asset_processor = None
        
    def teardown_class(cls):
        logger = logging.getLogger(__name__)
        # Report times
        time_info_str = "Individual test times (Full test time, Editor test time):\n"
        for testcase_name, t in cls.test_times.items():
            editor_t = cls.editor_times[testcase_name]
            time_info_str += f"{testcase_name}: (Full:{t} sec, Editor:{editor_t} sec)\n"
            
        logger.info(time_info_str)
        # Kill all ly processes
        cls.asset_processor.teardown()
        cls._kill_ly_processes()

    def _run_test(self, request, workspace, editor, testcase_module, extra_cmdline_args=[], batch_mode=True,
                  autotest_mode=True, use_null_renderer=True, enable_prefab_system=True):
        test_starttime = time.time()
        self.logger = logging.getLogger(__name__)
        errors = []
        testcase_name = os.path.basename(testcase_module.__file__)
        
        #########
        # Setup #
        
        if self.asset_processor is None:
            self.__class__.asset_processor = AssetProcessor(workspace)
            self.asset_processor.backup_ap_settings()
        
        self._kill_ly_processes(include_asset_processor=False)
        self.asset_processor.start()    
        self.asset_processor.wait_for_idle()

        def teardown():
            if os.path.exists(workspace.paths.editor_log()):
                workspace.artifact_manager.save_artifact(workspace.paths.editor_log())
            try:
                file_system.restore_backup(workspace.paths.editor_log(), workspace.paths.project_log())
            except FileNotFoundError as e:
                self.logger.debug(f"File restoration failed, editor log could not be found.\nError: {e}")
            editor.kill()

        request.addfinalizer(teardown)

        if os.path.exists(workspace.paths.editor_log()):
            self.logger.debug("Creating backup for existing editor log before test run.")
            file_system.create_backup(workspace.paths.editor_log(), workspace.paths.project_log())
            
        ############
        # Run test # 
        
        editor_starttime = time.time()
        self.logger.debug("Running automated test")
        testcase_module_filepath = self._get_testcase_module_filepath(testcase_module)
        pycmd = ["--runpythontest", testcase_module_filepath, f"-pythontestcase={request.node.name}"]
        if use_null_renderer:
            pycmd += ["-rhi=null"]
        if batch_mode:
            pycmd += ["-BatchMode"]
        if autotest_mode:
            pycmd += ["-autotest_mode"]
        if enable_prefab_system:
            pycmd += ["--regset=/Amazon/Preferences/EnablePrefabSystem=true"]
        else:
            pycmd += ["--regset=/Amazon/Preferences/EnablePrefabSystem=false"]

        pycmd += extra_cmdline_args
        editor.args.extend(pycmd) # args are added to the WinLauncher start command
        editor.start(backupFiles = False, launch_ap = False)
        try:
            editor.wait(TestAutomationBase.MAX_TIMEOUT)
        except WaitTimeoutError:
            errors.append(TestRunError("TIMEOUT", f"Editor did not close after {TestAutomationBase.MAX_TIMEOUT} seconds, verify the test is ending and the application didn't freeze"))
            editor.kill()
            
        output = editor.get_output()
        self.logger.debug("Test output:\n" + output)
        return_code = editor.get_returncode()
        
        self.editor_times[testcase_name] = time.time() - editor_starttime
        
        ###################
        # Validate result #
        
        if return_code != 0:
            if output:
                error_str = "Test failed, output:\n" + output.replace("\n", "\n  ")
            else:
                error_str = "Test failed, no output available..\n"
            errors.append(TestRunError("FAILED TEST", error_str))
            if return_code and return_code != TestAutomationBase.TEST_FAIL_RETCODE: # Crashed
                crash_info = "-- No crash log available --"
                crash_log = workspace.paths.crash_log()

                try:
                    waiter.wait_for(lambda: os.path.exists(crash_log), timeout=TestAutomationBase.WAIT_FOR_CRASH_LOG)
                except AssertionError:                    
                    pass
                    
                try:
                    with open(crash_log) as f:
                        crash_info = f.read()
                except Exception as ex:
                    crash_info += f"\n{str(ex)}"

                return_code_str = f"0x{return_code:0X}" if isinstance(return_code, int) else "None"
                error_str = f"Editor.exe crashed, return code: {return_code_str}\n\nCrash log:\n{crash_info}"
                errors.append(TestRunError("CRASH", error_str))
        
        self.test_times[testcase_name] = time.time() - test_starttime
        
        ###################
        # Error reporting #
        
        if errors:
            error_str = "Error list:\n"
            longest_title = max([len(e.title) for e in errors])
            longest_title += (longest_title % 2) # make it even spaces
            longest_title = max(30, longest_title) # at least 30 -
            header_decoration = "-".center(longest_title, "-") + "\n"
            for e in errors:
                error_str += header_decoration
                error_str += f"  {e.title}  ".center(longest_title, "-")  + "\n"
                error_str += header_decoration
                for line in e.content.split("\n"):
                    error_str += f"  {line}\n"
                
            error_str += header_decoration
            error_str += "Editor log:\n"
            try:
                with open(workspace.paths.editor_log()) as f:
                    log_basename = os.path.basename(workspace.paths.editor_log())
                    for line in f.readlines():
                        error_str += f"|{log_basename}| {line}"                        
            except Exception as ex:
                error_str += f"-- No log available ({ex})--"

            pytest.fail(error_str)     
        
    @staticmethod
    def _kill_ly_processes(include_asset_processor=True):
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
    def _get_testcase_module_filepath(testcase_module):
        # type: (Module) -> str
        """
        return the full path of the test module
        :param testcase_module: The testcase python module being tested
        :return str: The full path to the testcase module
        """
        return os.path.splitext(testcase_module.__file__)[0] + ".py"
