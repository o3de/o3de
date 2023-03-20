"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


def Atom_LevelLoadTest():
    """
    Summary:
    Loads all graphics levels within the AutomatedTesting project in editor. For each level this script will verify that
    the level loads, and can enter/exit gameplay without crashing the editor.

    Test setup:
    - Store all available levels in a list.
    - Set up a for loop to run all checks for each level.

    Expected Behavior:
    Test verifies that each level loads, enters/exits game mode, and reports success for all test actions.

    Test Steps for each level:
    1) Create tuple with level load success and failure messages
    2) Open the level using the python test tools command
    3) Verify level is loaded using a separate command, and report success/failure
    4) Enter gameplay and report result using a tuple
    5) Exit Gameplay and report result using a tuple
    6) Look for errors or asserts.

    :return: None
    """

    import azlmbr.legacy.general as general

    from editor_python_test_tools.utils import Report, Tracer, TestHelper
    from Atom.atom_utils.atom_constants import LEVEL_LIST

    with Tracer() as error_tracer:

        for level in LEVEL_LIST:

            # 1. Create tuple with level load success and failure messages
            level_check_tuple = (f"loaded {level}", f"failed to load {level}")

            # 2. Open the level using the python test tools command
            TestHelper.init_idle()
            TestHelper.open_level("Graphics", level)

            # 3. Verify level is loaded using a separate command, and report success/failure
            Report.result(level_check_tuple, level == general.get_current_level_name())

            # 4. Enter gameplay and report result using a tuple
            enter_game_mode_tuple = (f"{level} entered gameplay successfully ", f"{level} failed to enter gameplay")
            TestHelper.enter_game_mode(enter_game_mode_tuple)
            general.idle_wait_frames(1)

            # 5. Exit gameplay and report result using a tuple
            exit_game_mode_tuple = (f"{level} exited gameplay successfully ", f"{level} failed to exit gameplay")
            TestHelper.exit_game_mode(exit_game_mode_tuple)


        # 6. Look for errors or asserts.
        TestHelper.wait_for_condition(lambda: error_tracer.has_errors or error_tracer.has_asserts, 1.0)
        for error_info in error_tracer.errors:
            Report.info(f"Error: {error_info.filename} {error_info.function} | {error_info.message}")
        for assert_info in error_tracer.asserts:
            Report.info(f"Assert: {assert_info.filename} {assert_info.function} | {assert_info.message}")


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(Atom_LevelLoadTest)
