"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


# Test case ID : C4976207
# Test Case Title : Verify that when Kinematic is checked, the object behaves as a Kinematic object



# fmt: off
class Tests():
    enter_game_mode         = ("Entered game mode",             "Failed to enter game mode")
    find_box                = ("Box found",                     "Box not found")
    find_ramp               = ("Ramp found",                    "Ramp not found")
    box_is_not_kinematic    = ("Box is not kinematic",          "Box is kinematic")
    ramp_is_kinematic       = ("Ramp is kinematic",             "Ramp is not kinematic")
    box_gravity_enabled     = ("Gravity enabled on the box",    "Gravity not enabled on the box")
    ramp_gravity_enabled    = ("Gravity enabled on the ramp",   "Gravity not enabled on the ramp")
    box_touched_ramp        = ("Box touched ramp",              "Box did not touch ramp")
    ramp_did_not_move       = ("Ramp did not move",             "Ramp moved")
    exit_game_mode          = ("Exited game mode",              "Failed to exit game mode")
# fmt: on


def RigidBody_KinematicModeWorks():
    """
    Summary:
    Runs an automated test to ensure kinematic property causes entity to remain in place when gravity acts upon it
    and when another entity collides with it.

    Level Description:
    A box (entity: Box) set above a kinematic ramp (entity: Ramp). Gravity is enabled for the the box and the ramp.

    Expected behavior:
    The box collides with the ramp, and the ramp does not fall.

    Test Steps:
    1) Open level and enter game mode
    2) Retrieve entities
    3) Check for kinematic ramp and not kinematic box
    4) Check that gravity is enabled on the entities
    5) Get the initial position of the ramp
    6) Check to see that the box hits the ramp
        6.5) Wait for the box to touch the ramp or timeout
    7) Check to see that the ramp stayed at the same position
    8) Exit game mode and close editor

    Note:
    - This test file must be called from the Open 3D Engine Editor command terminal
    - Any passed and failed tests are written to the Editor.log file.
            Parsing the file or running a log_monitor are required to observe the test results.

    :return: None
    """
    # Setup path
    import os
    import sys
    import azlmbr.legacy.general as general
    import azlmbr.bus
    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper as helper

    # Specific wait times in seconds
    TIME_OUT = 3.0
    REACTION = 0.1

    # 1) Open level and enter game mode
    helper.init_idle()
    helper.open_level("Physics", "RigidBody_KinematicModeWorks")
    helper.enter_game_mode(Tests.enter_game_mode)

    # 2) Retrieve entities
    box_id = general.find_game_entity("Box")
    Report.result(Tests.find_box, box_id.IsValid())

    ramp_id = general.find_game_entity("Ramp")
    Report.result(Tests.find_ramp, ramp_id.IsValid())

    # 2.1) setup collision handler
    class RampTouched:
        value = False

    def on_collision_begin(args):
        other_id = args[0]
        if other_id.Equal(ramp_id):
            Report.info("Box touched ramp")
            RampTouched.value = True

    handler = azlmbr.physics.CollisionNotificationBusHandler()
    handler.connect(box_id)
    handler.add_callback("OnCollisionBegin", on_collision_begin)

    # 3) Check for kinematic ramp and not kinematic box
    box_kinematic = azlmbr.physics.RigidBodyRequestBus(azlmbr.bus.Event, "IsKinematic", box_id)
    Report.result(Tests.box_is_not_kinematic, not box_kinematic)

    ramp_kinematic = azlmbr.physics.RigidBodyRequestBus(azlmbr.bus.Event, "IsKinematic", ramp_id)
    Report.result(Tests.ramp_is_kinematic, ramp_kinematic)

    # 4) Check that gravity is enabled on the entities
    box_gravity_enabled = azlmbr.physics.RigidBodyRequestBus(azlmbr.bus.Event, "IsGravityEnabled", box_id)
    Report.result(Tests.box_gravity_enabled, box_gravity_enabled)

    ramp_gravity_enabled = azlmbr.physics.RigidBodyRequestBus(azlmbr.bus.Event, "IsGravityEnabled", ramp_id)
    Report.result(Tests.ramp_gravity_enabled, ramp_gravity_enabled)

    # 5) Get the initial position of the ramp
    ramp_pos_start = azlmbr.components.TransformBus(azlmbr.bus.Event, "GetWorldTranslation", ramp_id)
    Report.info("Ramp's initial position: {}".format(ramp_pos_start))

    # 6) Wait for the box to touch the ramp or timeout
    helper.wait_for_condition(lambda: RampTouched.value, TIME_OUT)
    Report.result(Tests.box_touched_ramp, RampTouched.value)

    # 7) Check to see that the ramp stayed at the same position
    general.idle_wait(REACTION)  # wait for collision reaction
    ramp_pos_end = azlmbr.components.TransformBus(azlmbr.bus.Event, "GetWorldTranslation", ramp_id)
    Report.info("Ramp's final position: {}".format(ramp_pos_end))
    Report.result(Tests.ramp_did_not_move, ramp_pos_start.Equal(ramp_pos_end))

    # 8) Exit game mode and close editor
    helper.exit_game_mode(Tests.exit_game_mode)


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(RigidBody_KinematicModeWorks)
