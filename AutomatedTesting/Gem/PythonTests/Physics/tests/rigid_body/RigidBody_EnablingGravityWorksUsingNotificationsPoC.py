"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

# Test case ID : C111111
# Test Case Title : Check that Gravity works

# fmt:off
class Tests:
    enter_game_mode          = ("Entered game mode",        "Failed to enter game mode")
    find_ball                = ("Entity Ball found",        "Ball not found")
    find_terrain             = ("Entity Terrain found",     "Terrain not found")
    gravity_started_disabled = ("Gravity started disabled", "Gravity didn't start disabled")
    gravity_set_enabled      = ("Gravity has been enabled", "Gravity wasn't enabled")
    ball_fell                = ("Ball fell",                "Ball didn't fall")
    exit_game_mode           = ("Exited game mode",         "Couldn't exit game mode")
# fmt:on


def RigidBody_EnablingGravityWorksUsingNotificationsPoC():
    # Setup path
    import os
    import sys
    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper as helper

    import azlmbr.legacy.general as general
    import azlmbr.bus

    helper.init_idle()

    # 1) Open level
    helper.open_level("Physics", "EnablingGravityWorks")

    # 2) Enter game mode
    helper.enter_game_mode(Tests.enter_game_mode)

    # 3) Retrieve entities
    ball_id = general.find_game_entity("Ball")
    Report.result(Tests.find_ball, ball_id.IsValid())

    terrain_id = general.find_game_entity("Terrain")
    Report.result(Tests.find_terrain, terrain_id.IsValid())

    # 4) Make sure gravity is off from the start
    gravity_enabled = azlmbr.physics.RigidBodyRequestBus(azlmbr.bus.Event, "IsGravityEnabled", ball_id)
    Report.result(Tests.gravity_started_disabled, not gravity_enabled)

    # 5) Activate gravity
    Report.info("Enabling Gravity")
    azlmbr.physics.RigidBodyRequestBus(azlmbr.bus.Event, "ForceAwake", ball_id)
    azlmbr.physics.RigidBodyRequestBus(azlmbr.bus.Event, "SetGravityEnabled", ball_id, True)

    gravity_enabled = azlmbr.physics.RigidBodyRequestBus(azlmbr.bus.Event, "IsGravityEnabled", ball_id)
    Report.result(Tests.gravity_set_enabled, gravity_enabled)

    # 6) Listen to collision events seconds so it falls down

    class TouchGround:
        value = False

    def on_collision_begin(args):
        other_id = args[0]
        if other_id.Equal(terrain_id):
            Report.info("Touched ground")
            TouchGround.value = True

    handler = azlmbr.physics.CollisionNotificationBusHandler()
    handler.connect(ball_id)
    handler.add_callback("OnCollisionBegin", on_collision_begin)

    helper.wait_for_condition(lambda: TouchGround.value, 3.0)
    Report.result(Tests.ball_fell, TouchGround.value)

    # 7) Exit game mode
    helper.exit_game_mode(Tests.exit_game_mode)

if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(RigidBody_EnablingGravityWorksUsingNotificationsPoC)
