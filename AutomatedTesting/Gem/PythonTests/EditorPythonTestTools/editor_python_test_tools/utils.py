"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import json
import math
import os
import time
import traceback
from typing import Callable, Tuple
from enum import Enum

import azlmbr
try:
    import azlmbr.atomtools.general as general  # Standard MaterialEditor or similar executable test.
except ModuleNotFoundError:  # azlmbr.atomtools is not yet available in the Editor
    import azlmbr.legacy.general as general  # Will be updated in https://github.com/o3de/o3de/issues/11056

try:
    import azlmbr.multiplayer as multiplayer
except ModuleNotFoundError: # Not all projects enable the Multiplayer Gem.
    pass

import azlmbr.debug
import ly_test_tools.environment.waiter as waiter
import ly_test_tools.environment.process_utils as process_utils

# Controls how many reconnection attempts to make from the editor to the multiplayer server before giving up and failing the test.
# Note that the editor will wait one additional second in between each attempt, so the final time will be
# 1 + 2 + 3 + ... + MULIPLAYER_SERVER_RECONNECT_ATTEMPTS seconds, which formulates to n(n+1)/2 where n is the number of attempts.
# For example, with the value of 35, the editor will wait 630 seconds (10.5 minutes) before giving up.

# also note that the editor stops trying to connect the moment it is successful so its better for automated testing to set this to a very
# large number rather than a borderline number.  10.5 minutes of still not appearing should be a very high confidence indicator that the 
# server is not going to appear even on a very busy machine, and it's better to have a reliable indicator of failure than to have a borderline
# wait time that could be failing just because it's a busy machine.
MULIPLAYER_SERVER_RECONNECT_ATTEMPTS = 35

class FailFast(Exception):
    """
    Raise to stop proceeding through test steps.
    """
    pass


class TestHelper:
    @staticmethod
    def init_idle():
        general.idle_enable(True)

    @staticmethod
    def create_level(level_name: str) -> bool:
        """
        :param level_name: The name of the level to be created
        :return: True if ECreateLevelResult returns 0, False otherwise with logging to report reason
        """
        template_name = "Prefabs/Default_Level.prefab"
        Report.info(f"Creating level {level_name} from template '{template_name}'")

        # Use these hardcoded values to pass expected values for old terrain system until new create_level API is
        # available
        heightmap_resolution = 1024
        heightmap_meters_per_pixel = 1
        terrain_texture_resolution = 4096
        use_terrain = False

        result = general.create_level_no_prompt(template_name, level_name, heightmap_resolution, heightmap_meters_per_pixel,
                                                terrain_texture_resolution, use_terrain)

        # Result codes are ECreateLevelResult defined in CryEdit.h
        if result == 1:
            Report.info(f"{level_name} level already exists")
        elif result == 2:
            Report.info("Failed to create directory")
        elif result == 3:
            Report.info("Directory length is too long")
        elif result != 0:
            Report.info("Unknown error, failed to create level")
        else:
            Report.info(f"{level_name} level created successfully")

        return result == 0

    @staticmethod
    def open_level(directory : str, level : str, no_prompt: bool = True):
        # type: (str, str) -> None
        """
        :param level: the name of the level folder in AutomatedTesting\\Physics\\

        :return: None
        """
        # Make sure we are not in game mode
        if general.is_in_game_mode():
            general.exit_game_mode()
            TestHelper.wait_for_condition(lambda : not general.is_in_game_mode(), 1.0)
            assert not general.is_in_game_mode(), "Editor was in gamemode when opening the level and was unable to exit from it"

        Report.info("Open level {}/{}".format(directory, level))
        if no_prompt:
            success = general.open_level_no_prompt(os.path.join(directory, level))
        else:
            success = general.open_level(os.path.join(directory, level))

        if not success:
            open_level_name = general.get_current_level_name()
            if open_level_name == level:
                Report.info("{} was already opened".format(level))
            else:
                assert False, "Failed to open level: {} does not exist or is invalid".format(level)
                
        # FIX-ME: Expose call for checking when has been finished loading and change this frame waiting
        # Jira: LY-113761
        general.idle_wait_frames(200)

    @staticmethod
    def enter_game_mode(msgtuple_success_fail: Tuple[str, str]) -> None:
        """
        :param msgtuple_success_fail: The tuple with the expected/unexpected messages for entering game mode.

        :return: None
        """
        Report.info("Entering game mode")
        general.enter_game_mode()
        
        TestHelper.wait_for_condition(lambda : general.is_in_game_mode(), 1.0)
        Report.critical_result(msgtuple_success_fail, general.is_in_game_mode())

    @staticmethod
    def find_line(window, line, print_infos):
        """
        Looks for an expected line in a list of tracer log lines
        :param window: The log's window name. For example, logs printed via script-canvas use the "Script" window. 
        :param line: The log message to search for. 
        :param print_infos: A list of PrintInfos collected by Tracer to search. Example options: your_tracer.warnings, your_tracer.errors, your_tracer.asserts, or your_tracer.prints 

        :return: True if the line is found, otherwise false.
        """
        for printInfo in print_infos:
            if printInfo.window == window.strip() and printInfo.message.strip() == line:
                return True
        return False

    @staticmethod
    def succeed_if_log_line_found(window, line, print_infos, time_out):
        """
        Looks for a line in a list of tracer log lines and reports success if found.
        :param window: The log's window name. For example, logs printed via script-canvas use the "Script" window. 
        :param line: The log message we're hoping to find.
        :param print_infos: A list of PrintInfos collected by Tracer to search. Example options: your_tracer.warnings, your_tracer.errors, your_tracer.asserts, or your_tracer.prints 
        :param time_out: The total amount of time to wait before giving up looking for the expected line.

        :return: No return value, but if the message is found, a successful critical result is reported; otherwise failure.
        """
        TestHelper.wait_for_condition(lambda : TestHelper.find_line(window, line, print_infos), time_out)
        Report.critical_result(("Found expected line: " + line, "Failed to find expected line: " + line), TestHelper.find_line(window, line, print_infos))

    @staticmethod
    def fail_if_log_line_found(window, line, print_infos, time_out):
        """
        Reports a failure if a log line in a list of tracer log lines is found.
        :param window: The log's window name. For example, logs printed via script-canvas use the "Script" window. 
        :param line: The log message we're hoping to not find.
        :param print_infos: A list of PrintInfos collected by Tracer to search. Example options: your_tracer.warnings, your_tracer.errors, your_tracer.asserts, or your_tracer.prints 
        :param time_out: The total amount of time to wait before giving up looking for the unexpected line. If time runs out and we don't see the unexpected line then report a success.

        :return: No return value, but if the line is found, a failed critical result is reported; otherwise success.
        """
        TestHelper.wait_for_condition(lambda : TestHelper.find_line(window, line, print_infos), time_out)
        Report.critical_result(("Unexpected line not found: " + line, "Unexpected line found: " + line), not TestHelper.find_line(window, line, print_infos))

    @staticmethod
    def all_expected_log_lines_found(section_tracer, lines):
        """
        function for parsing game mode's console output for expected test lines. duplicate lines and error lines are not
        handled by this function.

        param section_tracer: python editor tracer object
        param lines: list of expected lines


        returns true if all the expected lines were detected in the parsed output
        """
        found_lines = [printInfo.message.strip() for printInfo in section_tracer.prints]

        expected_lines = len(lines)
        matching_lines = 0

        for line in lines:
            for found_line in found_lines:
                if line == found_line:
                    matching_lines += 1

        return matching_lines >= expected_lines

    
    EditorServerMode = Enum('EditorServerMode', ['DEDICATED_SERVER', 'CLIENT_SERVER'])

    @staticmethod
    def multiplayer_enter_game_mode(msgtuple_success_fail: Tuple[str, str], editor_server_mode: EditorServerMode) -> None:
        """
        :param msgtuple_success_fail: The tuple with the expected/unexpected messages for entering game mode.

        :return: None
        """
        Report.info("Entering game mode")

        with MultiplayerHelper() as multiplayer_helper:
            # enter game-mode. 
            # game-mode in multiplayer will also launch ServerLauncher.exe and connect to the editor
            general.set_cvar_integer('editorsv_max_connection_attempts', MULIPLAYER_SERVER_RECONNECT_ATTEMPTS)

            # The editor waits 1 additional second between each retry, meaning the total time is the sum of the first n natural numbers, which
            # solves as n(n+1)/2.
            wait_duration = MULIPLAYER_SERVER_RECONNECT_ATTEMPTS * (MULIPLAYER_SERVER_RECONNECT_ATTEMPTS + 1.0) / 2.0

            general.set_cvar_string('editorsv_clientserver', 'true' if editor_server_mode == TestHelper.EditorServerMode.CLIENT_SERVER else 'false')

            multiplayer.PythonEditorFuncs_enter_game_mode()

            if editor_server_mode == TestHelper.EditorServerMode.DEDICATED_SERVER:
                # note that this next line merely waits for the editor say that it has asked the server to launch, not that it did actually launch.
                TestHelper.wait_for_condition(lambda : multiplayer_helper.serverLaunched, 20.0)

                # note that this next line ensures that the process is running, not that it is connected to the editor.
                waiter.wait_for(lambda: process_utils.process_exists("AutomatedTesting.ServerLauncher", ignore_extensions=True), timeout=5.0, exc=AssertionError("AutomatedTesting.ServerLauncher process is not running!"), interval=1.0)
                Report.critical_result(("AutomatedTesting.ServerLauncher process successfully launched", "AutomatedTesting.ServerLauncher process failed to launch"), process_utils.process_exists("AutomatedTesting.ServerLauncher", ignore_extensions=True))

                # note that this line waits for the editor to say that it has attempted to connect to the server at least once, not that it has actually connected.
                TestHelper.wait_for_condition(lambda : multiplayer_helper.editorConnectionAttemptCount > 0, 10.0)
                Report.critical_result(("Multiplayer Editor attempting server connection.", "Multiplayer Editor never tried connecting to the server."), multiplayer_helper.editorConnectionAttemptCount > 0)

                # between the previous step and this one, the editor has to use up all its connection attempts, and be successfully connected, which can take a while
                # since the server process has to actually start up, load all its dynamic libraries, assets, etc.
                # so it has to wait at least wait_duration seconds or else the test will fail prematurely
                TestHelper.wait_for_condition(lambda : multiplayer_helper.editorSendingLevelData, wait_duration)
                Report.critical_result(("Multiplayer Editor sent level data to the server.", "Multiplayer Editor never sent the level to the server."), multiplayer_helper.editorSendingLevelData)

                TestHelper.wait_for_condition(lambda : multiplayer_helper.connectToSimulationSuccess, 20.0)
                Report.critical_result(("Multiplayer Editor successfully connected to server network simuluation.", "Multiplayer Editor failed to connected to server network simuluation."), multiplayer_helper.connectToSimulationSuccess)

        TestHelper.wait_for_condition(lambda : multiplayer.PythonEditorFuncs_is_in_game_mode(), 10.0)
        Report.critical_result(msgtuple_success_fail, multiplayer.PythonEditorFuncs_is_in_game_mode())

    @staticmethod
    def exit_game_mode(msgtuple_success_fail : Tuple[str, str]):
        # type: (tuple) -> None
        """
        :param msgtuple_success_fail: The tuple with the expected/unexpected messages for exiting game mode.

        :return: None
        """
        Report.info("Exiting game mode")
        general.exit_game_mode()

        TestHelper.wait_for_condition(lambda : not general.is_in_game_mode(), 1.0)
        Report.critical_result(msgtuple_success_fail, not general.is_in_game_mode())

    @staticmethod
    def close_editor():
        general.exit_no_prompt()

    @staticmethod
    def fail_fast(message=None):
        # type: (str) -> None
        """
        A state has been reached where progressing in the test is not viable.
        raises FailFast
        :return: None
        """
        if message:
            Report.info("Fail fast message: {}".format(message))
        raise FailFast()

    @staticmethod
    def wait_for_condition(function, timeout_in_seconds):
        # type: (function, float) -> bool
        """
        **** Will be replaced by a function of the same name exposed in the Engine*****
        a function to run until it returns True or timeout is reached
        the function can have no parameters and
        waiting idle__wait_* is handled here not in the function

        :param function: a function that returns a boolean indicating a desired condition is achieved
        :param timeout_in_seconds: when reached, function execution is abandoned and False is returned
        """

        with Timeout(timeout_in_seconds) as t:
            while True:
                try:
                    general.idle_wait_frames(1)
                except:
                    Report.info("WARNING: Couldn't wait for frame")
                    
                if t.timed_out:
                    return False

                ret = function()
                if not isinstance(ret, bool):
                    raise TypeError("return value for wait_for_condition function must be a bool")
                if ret:
                    return True

    @staticmethod
    def close_error_windows():
        """
        Closes Error Report and Error Log windows that block focus if they are visible.
        :return: None
        """
        if general.is_pane_visible("Error Report"):
            general.close_pane("Error Report")
        if general.is_pane_visible("Error Log"):
            general.close_pane("Error Log")

    @staticmethod
    def close_display_helpers():
        """
        Closes helper gizmos, anti-aliasing, and FPS meters.
        :return: None
        """
        if general.is_helpers_shown():
            general.toggle_helpers()
            general.idle_wait(1.0)
        general.idle_wait(1.0)
        general.run_console("r_displayInfo=0")
        general.idle_wait(1.0)


class Timeout:
    # type: (float) -> None
    """
    contextual timeout
    :param seconds: float seconds to allow before timed_out is True
    """

    def __init__(self, seconds):
        self.seconds = seconds

    def __enter__(self):
        self.die_after = time.time() + self.seconds
        return self

    def __exit__(self, type, value, traceback):
        pass

    @property
    def timed_out(self):
        return time.time() > self.die_after


class Report:
    _results = []
    _exception = None

    @staticmethod
    def start_test(test_function : Callable):
        """
        Runs the test, outputs the report and asserts in case of failure.
        @param: The test function to execute
        """
        Report._results = []
        Report._exception = None
        general.test_output(f"Starting test {test_function.__name__}...\n")
        try:
            test_function()
        except Exception as ex:
            Report._exception = traceback.format_exc()

        success, report_str = Report.get_report(test_function)
        # Print on the o3de console, for debugging purposes
        print(report_str)
        # Print the report on the piped stdout of the application
        general.test_output(report_str)
        assert success, f"Test {test_function.__name__} failed"

    @staticmethod
    def get_report(test_function : Callable) -> (bool, str):
        """
        Outputs infomation on the editor console for the test
        @param msg: message to output
        @return: (success, report_information) tuple
        """
        success = True
        report = f"Test {test_function.__name__} finished.\nReport:\n"
        # report_dict is a JSON that can be used to parse test run information from a external runner
        # The regular report string is intended to be used for manual debugging
        filename = os.path.splitext(os.path.basename(test_function.__code__.co_filename))[0] 
        report_dict = {'name' : filename, 'success' : True, 'exception' : None}
        for result in Report._results:
            passed, info = result
            success = success and passed
            test_result_info = ""
            if passed:
                test_result_info = f"[SUCCESS] {info}"
            else:
                test_result_info = f"[FAILED ] {info}"
            report += f"{test_result_info}\n"
        if Report._exception:
            exception_str = Report._exception[:-1].replace("\n", "\n  ")
            report += "EXCEPTION raised:\n  %s\n" % exception_str
            report_dict['exception'] = exception_str
            success = False
        report += "Test result:  " + ("SUCCESS" if success else "FAILURE")
        report_dict['success'] = success
        report_dict['output'] = report
        report_json_str = json.dumps(report_dict)
        # For helping parsing, the json will be always contained between JSON_START JSON_END
        report += f"\nJSON_START({report_json_str})JSON_END\n"
        return success, report

    @staticmethod
    def info(msg : str):
        """
        Outputs infomation on the editor console for the test
        @param msg: message to output
        """
        print("Info: {}".format(msg))

    @staticmethod
    def success(msgtuple_success_fail : Tuple[str, str]):
        """
        Given a test string tuple (success_string, failure_string), registers the test result as success
        @param msgtuple_success_fail: Two element tuple of success and failure strings
        """
        outcome = "Success: {}".format(msgtuple_success_fail[0])
        print(outcome)
        Report._results.append((True, outcome))

    @staticmethod
    def failure(msgtuple_success_fail : Tuple[str, str]):
        """
        Given a test string tuple (success_string, failure_string), registers the test result as failed
        @param msgtuple_success_fail: Two element tuple of success and failure strings
        """
        outcome = "Failure: {}".format(msgtuple_success_fail[1])
        print(outcome)
        Report._results.append((False, outcome))

    @staticmethod
    def result(msgtuple_success_fail : Tuple[str, str], outcome : bool):
        """
        Given a test string tuple (success_string, failure_string), registers the test result based on the
        given outcome
        @param msgtuple_success_fail: Two element tuple of success and failure strings
        @param outcome: True or False if the result has been a sucess or failure
        """
        if not isinstance(outcome, bool):
            raise TypeError("outcome argument must be a bool")

        if outcome:
            Report.success(msgtuple_success_fail)
        else:
            Report.failure(msgtuple_success_fail)
        return outcome

    @staticmethod
    def critical_result(msgtuple_success_fail : Tuple[str, str], outcome : bool, fast_fail_message : str = None):
        # type: (tuple, bool, str) -> None
        """
        if outcome is False we will fail fast

        :param msgtuple_success_fail: messages to print based on the outcome
        :param outcome: success (True) or failure (False)
        :param fast_fail_message: [optional] message to include on fast fail
        """
        if not isinstance(outcome, bool):
            raise TypeError("outcome argument must be a bool")

        if not Report.result(msgtuple_success_fail, outcome):
            TestHelper.fail_fast(fast_fail_message)

    # DEPRECATED: Use vector3_str()
    @staticmethod
    def info_vector3(vector3 : azlmbr.math.Vector3, label : str ="", magnitude : float =None):
        # type: (azlmbr.math.Vector3, str, float) -> None
        """
        prints the vector to the Report.info log. If applied, label will print first,
        followed by the vector's values (x, y, z,) to 2 decimal places. Lastly if the
        magnitude is supplied, it will print on the third line.

        :param vector3: a azlmbr.math.Vector3 object to print
                    prints in [x: , y: , z: ] format.
        :param label: [optional] A string to print before printing the vector3's contents
        :param magnitude: [optional] the vector's magnitude to print after the vector's contents
        :return: None
        """
        if label != "":
            Report.info(label)
        Report.info("   x: {:.2f}, y: {:.2f}, z: {:.2f}".format(vector3.x, vector3.y, vector3.z))
        if magnitude is not None:
            Report.info("   magnitude: {:.2f}".format(magnitude))


'''
Utility for scope tracing errors and warnings.
Usage:

    ...
    with Tracer() as section_tracer:
        # section were we are interested in capturing errors/warnings/asserts
        ...

    Report.result(Tests.warnings_not_found_in_section, not section_tracer.has_warnings)

'''
class Tracer:
    def __init__(self):
        self.warnings = []
        self.errors = []
        self.asserts = []
        self.prints = []
        self.has_warnings = False
        self.has_errors = False
        self.has_asserts = False
        self.handler = None
    
    class WarningInfo:
        def __init__(self, args):
            self.window = args[0]
            self.filename = args[1]
            self.line = args[2]
            self.function = args[3]
            self.message = args[4]
            
        def __str__(self):
            return f"Warning: [{self.filename}:{self.function}:{self.line}]: [{self.window}] {self.message}"
            
        def __repr__(self):
            return f"[Warning: {self.message}]"
            
    class ErrorInfo:
        def __init__(self, args):
            self.window = args[0]
            self.filename = args[1]
            self.line = args[2]
            self.function = args[3]
            self.message = args[4]
            
        def __str__(self):
            return f"Error: [{self.filename}:{self.function}:{self.line}]: [{self.window}] {self.message}"
        
        def __repr__(self):
            return f"[Error: {self.message}]"
    
    class AssertInfo:
        def __init__(self, args):
            self.filename = args[0]
            self.line = args[1]
            self.function = args[2]
            self.message = args[3]

        def __str__(self):
            return f"Assert: [{self.filename}:{self.function}:{self.line}]: {self.message}"

        def __repr__(self):
            return f"[Assert: {self.message}]"

    class PrintInfo:
        def __init__(self, args):
            self.window = args[0]
            self.message = args[1]

    def _on_warning(self, args):
        warningInfo = Tracer.WarningInfo(args)
        self.warnings.append(warningInfo)
        Report.info("Tracer caught Warning: %s" % warningInfo.message)
        self.has_warnings = True
        return False

    def _on_error(self, args):
        errorInfo = Tracer.ErrorInfo(args)
        self.errors.append(errorInfo)
        Report.info("Tracer caught Error: %s" % errorInfo.message)
        self.has_errors = True
        return False

    def _on_assert(self, args):
        assertInfo = Tracer.AssertInfo(args)
        self.asserts.append(assertInfo)
        Report.info("Tracer caught Assert: %s:%i[%s] \"%s\"" % (assertInfo.filename, assertInfo.line, assertInfo.function, assertInfo.message))
        self.has_asserts = True
        return False

    def _on_printf(self, args):
        printInfo = Tracer.PrintInfo(args)
        self.prints.append(printInfo)
        return False

    def __enter__(self):
        self.handler = azlmbr.debug.TraceMessageBusHandler()
        self.handler.connect(None)
        self.handler.add_callback("OnPreAssert", self._on_assert)
        self.handler.add_callback("OnPreWarning", self._on_warning)
        self.handler.add_callback("OnPreError", self._on_error)
        self.handler.add_callback("OnPrintf", self._on_printf)
        return self
        
    def __exit__(self, type, value, traceback):
        self.handler.disconnect()
        self.handler = None
        return False


class AngleHelper:
    @staticmethod
    def is_angle_close(x_rad, y_rad, tolerance):
        # type: (float, float , float) -> bool
        """
        compare if 2 angles measured in radians are close

        :param x_rad: angle in radians
        :param y_rad: angle in radians
        :param tolerance: the tolerance to define close
        :return: bool
        """
        sinx_sub_siny = math.sin(x_rad) - math.sin(y_rad)
        cosx_sub_cosy = math.cos(x_rad) - math.cos(y_rad)
        r = sinx_sub_siny * sinx_sub_siny + cosx_sub_cosy * cosx_sub_cosy
        diff = math.acos((2.0 - r) / 2.0)
        return abs(diff) <= tolerance

    @staticmethod
    def is_angle_close_deg(x_deg, y_deg, tolerance):
        # type: (float, float , float) -> bool
        """
        compare if 2 angles measured in degrees are close

        :param x_deg: angle in degrees
        :param y_deg: angle in degrees
        :param tolerance: the tolerance to define close
        :return: bool
        """
        return AngleHelper.is_angle_close(math.radians(x_deg), math.radians(y_deg), tolerance)

'''
Utility for receiving Multiplayer Editor notifications.
Usage:

    ...
    with MultiplayerHelper() as multiplayer_helper:
        # section were we are interested in capturing multiplayer editor notifications
        TestHelper.wait_for_condition(lambda : multiplayer_helper.serverLaunched, 10.0)
        ...
'''
class MultiplayerHelper:
    def __init__(self):
        self.handler = None
        self.serverLaunched = False
        self.editorConnectionAttemptCount = 0
        self.editorSendingLevelData = False
        self.connectToSimulationSuccess = False

    def __enter__(self):
        self.handler = azlmbr.multiplayer.MultiplayerEditorServerNotificationBusHandler()
        self.handler.connect()
        self.handler.add_callback("OnServerLaunched", self._on_server_launched)
        self.handler.add_callback("OnEditorConnectionAttempt", self._on_editor_connection_attempt)
        self.handler.add_callback("OnEditorSendingLevelData", self._on_editor_sending_level_data)
        self.handler.add_callback("OnConnectToSimulationSuccess", self._on_connect_to_simulation_success)
        return self
        
    def __exit__(self, type, value, traceback):
        self.handler.disconnect()
        self.handler = None
        return False

    def _on_server_launched(self, args):
        self.serverLaunched = True
        return False
    
    def _on_editor_connection_attempt(self, args):
        self.editorConnectionAttemptCount = args[0]
        return False
    
    def _on_editor_sending_level_data(self, args):
        self.editorSendingLevelData = True
        return False

    def _on_connect_to_simulation_success(self, args):
        self.connectToSimulationSuccess = True
        return False

def vector3_str(vector3):
    return "(x: {:.2f}, y: {:.2f}, z: {:.2f})".format(vector3.x, vector3.y, vector3.z)


def aabb_str(aabb):
    return "[Min: %s, Max: %s]" % (vector3_str(aabb.min), vector3_str(aabb.max))
