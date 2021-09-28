"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


# Test case ID : C6274125
# Test Case Title : Verify ScriptCanvas Trigger Events.



# fmt: off
class Tests():
    enter_game_mode          = ("Entered game mode",               "Failed to enter game mode")
    find_box                 = ("Box entity found",                "Box entity not found")
    find_sphere              = ("Sphere entity found",             "Sphere entity not found")
    sphere_enter_triggerarea = ("Sphere entered the trigger area", "Sphere did not entered trigger area")
    sphere_exit_triggerarea  = ("Sphere exited the trigger area",  "Sphere did not exited the trigger area")
    exit_game_mode           = ("Exited game mode",                "Couldn't exit game mode")
# fmt: on


class LogLines:
    """
    These lines are added to the expected lines in the test suite.
    """

    expected_lines = [
        "Scriptcanvas:Sphere entered the trigger Area",
        "Scriptcanvas:Sphere exited the trigger Area",
    ]


def ScriptCanvas_TriggerEvents():
    """
    Summary:
    Verify ScriptCanvas Trigger Events.

    Level Description:
    BoxCollider (entity) - Entity contains PhysX Collider (Box shape) and trigger is enabled for it.
    ScriptCanvas is attached to the PhysX box shape collider entity. ScriptCanvas has two trigger events.
    ScriptCanvas trigger events are OnTriggerEnter and OnTriggerExit.
    Sphere (entity)      - Entity contains PhysX Collider(Sphere shape) with PhysX Rigid Body.

    Expected Behavior:
    When game mode is entered, Sphere should enter and exit from ScriptCanvas Trigger area. ScriptCanvas prints
    the name of the entity when it enters and exits the ScriptCanvas trigger area.

    Test Steps:
     1) Open level
     2) Enter game mode
     3) Retrieve and validate entities
     4) Verify Sphere enter and exit from ScriptCanvas Trigger area
     5) Exit game mode
     6) Close the editor

    Note:
    - This test file must be called from the Open 3D Engine Editor command terminal
    - Any passed and failed tests are written to the Editor.log file.
            Parsing the file or running a log_monitor are required to observe the test results.

    :return: None
    """

    import os
    import sys
    import azlmbr.legacy.general as general
    import azlmbr.bus

    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper as helper

    helper.init_idle()

    # Constants
    TIMEOUT = 2.5

    # 1) Open level
    helper.open_level("Physics", "ScriptCanvas_TriggerEvents")

    # 2) Enter game mode
    helper.enter_game_mode(Tests.enter_game_mode)

    # 3) Retrieve and validate entities
    box_id = general.find_game_entity("Box")
    Report.critical_result(Tests.find_box, box_id.IsValid())

    sphere_id = general.find_game_entity("Sphere")
    Report.critical_result(Tests.find_sphere, sphere_id.IsValid())

    # 4) Verify Sphere enter and exit from ScriptCanvas Trigger area
    class BoxTrigger:
        entered = False
        exited = False

    def on_enter(args):
        other_id = args[0]
        if other_id.Equal(sphere_id):
            Report.info("Trigger entered")
            BoxTrigger.entered = True

    def on_exit(args):
        other_id = args[0]
        if other_id.Equal(sphere_id):
            Report.info("Trigger exited")
            BoxTrigger.exited = True

    handler = azlmbr.physics.TriggerNotificationBusHandler()
    handler.connect(box_id)
    handler.add_callback("OnTriggerEnter", on_enter)
    handler.add_callback("OnTriggerExit", on_exit)

    helper.wait_for_condition(lambda: BoxTrigger.entered, TIMEOUT)
    Report.result(Tests.sphere_enter_triggerarea, BoxTrigger.entered)
    helper.wait_for_condition(lambda: BoxTrigger.exited, TIMEOUT)
    Report.result(Tests.sphere_exit_triggerarea, BoxTrigger.exited)

    # 5) Exit game mode
    helper.exit_game_mode(Tests.exit_game_mode)



if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(ScriptCanvas_TriggerEvents)
