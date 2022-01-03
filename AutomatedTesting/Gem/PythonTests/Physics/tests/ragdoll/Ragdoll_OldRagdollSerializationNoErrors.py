"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

"""
Test case ID : C14654882
Test Case Title : Loading level with old PhysX Ragdoll component serialization should not produce asset processor errors

"""


# fmt: off
class Tests:
    enter_game_mode          = ("Entered game mode",                                     "Failed to enter game mode")
    exit_game_mode           = ("Exited game mode",                                      "Failed to exit game mode")
    log_modified             = ("The log has been modified from the test run",           "The test run failed to modify the logs")
    length_of_recorded_lines = ("Log lines were grabbed for checking",                   "There were no lines grabbed for checking")
    error_line_not_found     = ("Specified lines were not found in the AP_GUI log file", "One or more specified lines were found in the AP_GUI log file")
# fmt: on


class ExpectedLines:
    # These lines are exported to the test runner code to be used via the test suite log monitor
    lines = []


class UnexpectedLines:
    # These lines are exported to the test runner code to be used via the test suite log monitor
    lines = ["Assert", "RagdollComponent' is not registered with the serializer"]


def Ragdoll_OldRagdollSerializationNoErrors():
    """
    Summary:
        Opens level containing old PhysX Ragdoll component.

    Level Description:
        Ragdoll - (entity): PhysX Ragdoll:
                                Position iteration count: 16
                                Velocity iteration count: 8
                                Enable joint projection: checked
                                Joint projection linear tolerance: 0.001
                                Joint projection angular tolerance: 1.0 degrees

    Expected Behavior:
        The level should load and not produce any errors for the asset processor component
        serialization of the old PhysX Ragdoll.

    Test Steps
    0) Get the latest modified time for the AP_GUI.log
    1) Open the level
    2) Enter game mode
    3) Exit game mode
    4) Close the editor (Logs will be read after closing editor)
    5) Check the AP_GUI log file for the error
        5.1) Opens the log file to record the lines
        5.2) Check that recorded_lines size > 0
        5.3) Search the recorded lines for an Unexpected Line

    Note:
    - This test file must be called from the Open 3D Engine Editor command terminal
    - Any passed and failed tests are written to the Editor.log file.
        Parsing the file or running a log_monitor are required to observe the test results.

    :return: None
    """
    import os
    import sys
    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper as helper

    helper.init_idle()

    # 0) Get the latest modified time for the AP_GUI.log
    AP_GUI_log_path = os.path.join(os.getcwd(), "Bin64vc141", "logs", "AP_GUI.log")
    log_modified_time = None
    if os.path.exists(AP_GUI_log_path):
        log_modified_time = os.stat(AP_GUI_log_path).st_mtime

    # 1) Open the level
    helper.open_level("Physics", "Ragdoll_OldRagdollSerializationNoErrors")

    # 2) Enter game mode
    helper.enter_game_mode(Tests.enter_game_mode)

    # 3) Exit game mode
    helper.exit_game_mode(Tests.exit_game_mode)

    # 5) Check the AP_GUI log file for the error
    recorded_lines = []
    if log_modified_time:
        log_modified_time_final = os.stat(AP_GUI_log_path).st_mtime
        Report.info("Log modified time: {}".format(log_modified_time))
        Report.info("Log final modified time: {}".format(log_modified_time))
        Report.critical_result(
            Tests.log_modified, log_modified_time_final > log_modified_time, "The log has not updated since last run."
        )

    # 5.1) Opens the log file to read the lines
    with open(AP_GUI_log_path) as log_file:
        for line in log_file:
            if "~none~" in line:  # "~none~" in line with the message for starting a new AzFramework File Logging run
                recorded_lines = []  # clear the recorded lines to look for the newer lines
            else:
                recorded_lines.append(line)

    # The end of log file was reached which should produce the lines for the newest run
    # 5.2) Check that recorded_lines size > 0
    size = len(recorded_lines)
    Report.result(Tests.length_of_recorded_lines, size > 0)

    # 5.3) Search the recorded lines for an Unexpected Line
    error_found = False
    for line in recorded_lines:
        if any(s in line for s in UnexpectedLines.lines):
            error_found = True
            Report.info("Unexpected line was found:")
            Report.info(line)

    Report.result(Tests.error_line_not_found, not error_found)


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(Ragdoll_OldRagdollSerializationNoErrors)
