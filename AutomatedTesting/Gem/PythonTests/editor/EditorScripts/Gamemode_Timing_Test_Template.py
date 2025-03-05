"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

Target_Level = ""

class Tests:
    target_level_load = (
        f'{Target_Level} level loaded',
        f'{Target_Level} level failed to load')
    target_level_enter_game_mode = (
        f'{Target_Level} entered gameplay successfully',
        f'{Target_Level} failed to enter gameplay')
    target_level_exit_game_mode = (
        f'{Target_Level} exited gameplay successfully',
        f'{Target_Level} failed to exit gameplay')
    timeout_status = (
        "Timeout not reached, continuing test",
        "Warning - timeout reached, failing test")
    empty_level_load = (
        "Empty level loaded successfully, Editor remains stable",
        "P0: Empty level fails to load, editor is either hung or crashed")


def Atom_Enter_Exit_Game_Time_For_Taget_Level():
    """
    Summary:
    Loads the "Target_Level" defined above within an instance of Editor.exe using the null renderer. Test verifies that
    the level loads, can enter/exit gameplay, and that Editor.exe remains stable throughout this process.

    Test setup:
    - Launch Editor.exe

    Expected Behavior:
    Test verifies that level loads, enters/exits game mode, and editor remains stable.

    Test Steps:
    1) Load level, confirm that correct level is loaded, and report results
    2) Start timer for enter game mode.
    3) Validate that editor can enter gameplay successfully
    4) Checks if the timeout has been reached (default 10 sec), if not report time editor took to enter game mode.
    5) Start timer for exit game mode
    6) Validate that editor can exit gameplay successfully
    7) Checks if the timeout has been reached (default 10 sec), if not report time editor took to exit game mode.
    8) Open an empty level to verify editor remains stable
    9) Look for errors or asserts.

    :return: None
    """

    import azlmbr.legacy.general as general
    import time

    from editor_python_test_tools.utils import Report, Tracer, TestHelper

    # Timer class to track enter/exit game time
    class Timer:
        unit_divisor = 60
        hour_divisor = unit_divisor * unit_divisor

        def start(self):
            self._start_time = time.perf_counter()

        def stop(self):
            self._stop_time = time.perf_counter() - self._start_time
            return self._stop_time

        def log_time(self, message):
            elapsed_time = time.perf_counter() - self._start_time
            hours = int(elapsed_time / Timer.hour_divisor)
            minutes = int(elapsed_time % Timer.hour_divisor / Timer.unit_divisor)
            seconds = elapsed_time % Timer.unit_divisor

            return(f'{message}: {hours:0>2d}:{minutes:0>2d}:{seconds:0>5.2f}\n')

        def just_seconds(self):
            elapsed_time = time.perf_counter() - self._start_time
            seconds = elapsed_time % Timer.unit_divisor

            return seconds

    with Tracer() as error_tracer:

        # 1. Load target level, confirm that correct level is loaded, and report results.
        TestHelper.init_idle()
        TestHelper.open_level("Graphics", Target_Level)
        Report.result(Tests.target_level_load, Target_Level
                      == general.get_current_level_name())

        # 2. Start timer for enter game mode.
        enter_gamemode_length = Timer()
        enter_gamemode_length.start()

        # 3. Validate that editor can enter gameplay successfully.
        TestHelper.enter_game_mode(Tests.target_level_enter_game_mode)

        # 4. Checks if the timeout has been reached (default 10 sec), if not report time editor took to enter game mode.
        Report.critical_result(Tests.timeout_status, enter_gamemode_length.stop() < 10.0)

        Report.success((enter_gamemode_length.log_time
                        ("Enter game mode load time, format - Days:Hours:Seconds:Milliseconds"), "Timer Failed"))

        # 5. Start timer for exit game mode
        exit_gamemode_length = Timer()
        exit_gamemode_length.start()

        # 6. Validate that editor can exit gameplay successfully.
        TestHelper.exit_game_mode(Tests.target_level_exit_game_mode)

        # 7. Checks if the timeout has been reached (default 10 sec), if not report time editor took to exit game mode.
        Report.critical_result(Tests.timeout_status, exit_gamemode_length.stop() < 10.0)

        Report.success((exit_gamemode_length.log_time
                        ("Exit game mode load time, format - Days:Hours:Seconds:Milliseconds"), "Timer Failed"))

        # 8. Open an empty level to verify editor remains stable.
        TestHelper.open_level("Graphics", "base_empty")
        Report.result(Tests.empty_level_load, "base_empty" == general.get_current_level_name())

        # 9. Look for errors or asserts.
        TestHelper.wait_for_condition(lambda: error_tracer.has_errors or error_tracer.has_asserts, 1.0)
        for error_info in error_tracer.errors:
            Report.info(f"Error: {error_info.filename} {error_info.function} | {error_info.message}")
        for assert_info in error_tracer.asserts:
            Report.info(f"Assert: {assert_info.filename} {assert_info.function} | {assert_info.message}")


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(Atom_Enter_Exit_Game_Time_For_Taget_Level)