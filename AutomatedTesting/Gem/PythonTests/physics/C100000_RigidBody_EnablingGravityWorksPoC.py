"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
"""


# Test case ID : C100000
# Test Case Title : Check that Gravity works
# URL of the test case : https://testrail.agscollab.com/index.php?/cases/view/100000


# fmt:off
class Tests():
    enter_game_mode          = ("Entered game mode",        "Failed to enter game mode")
    find_ball                = ("Entity Ball found",        "Ball not found")
    gravity_started_disabled = ("Gravity started disabled", "Gravity didn't start disabled")
    gravity_set_enabled      = ("Gravity has been enabled", "Gravity wasn't enabled")
    ball_fell                = ("Ball fell",                "Ball didn't fall")
    exit_game_mode           = ("Exited game mode",         "Couldn't exit game mode")
# fmt:on


def C100000_RigidBody_EnablingGravityWorksPoC():
    import os
    import sys

    import ImportPathHelper as imports

    imports.init()

    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper as helper

    import azlmbr.legacy.general as general
    import azlmbr.bus

    helper.init_idle()
    helper.open_level("Physics", "EnablingGravityWorks")
    helper.enter_game_mode(Tests.enter_game_mode)

    # 3) Retrieve entities
    general.idle_wait_frames(1)
    ball_id = general.find_game_entity("Ball")
    Report.critical_result(Tests.find_ball, ball_id.IsValid(), "Entity must be found")

    # 4) Make sure gravity is off from the start
    gravity_enabled = azlmbr.physics.RigidBodyRequestBus(azlmbr.bus.Event, "IsGravityEnabled", ball_id)
    Report.critical_result(Tests.gravity_started_disabled, not gravity_enabled)

    # 5) Get the Z position before enabling the physics
    class Ball:
        z_start = None

    Ball.z_start = azlmbr.components.TransformBus(azlmbr.bus.Event, "GetWorldZ", ball_id)

    # 6) Activate gravity
    Report.info("Enabling Gravity")
    azlmbr.physics.RigidBodyRequestBus(azlmbr.bus.Event, "ForceAwake", ball_id)
    azlmbr.physics.RigidBodyRequestBus(azlmbr.bus.Event, "SetGravityEnabled", ball_id, True)
    gravity_enabled = azlmbr.physics.RigidBodyRequestBus(azlmbr.bus.Event, "IsGravityEnabled", ball_id)
    
    def ball_fell():
        """
        This is an example function to use with TestHelper.wait_for_condition
        It may take no parameters and it contains no wait_idle_* because that is
        already handled in TestHelper.wait_for_condition
        """
        z_end = azlmbr.components.TransformBus(azlmbr.bus.Event, "GetWorldZ", ball_id)
        return Ball.z_start > z_end

    # 7) Validate ball fell by ensuring z is decreasing
    fell_down = helper.wait_for_condition(ball_fell, 1.0)
    Report.result(Tests.ball_fell, fell_down)

    helper.exit_game_mode(Tests.exit_game_mode)

if __name__ == "__main__":
    import ImportPathHelper as imports
    imports.init()

    from editor_python_test_tools.utils import Report
    Report.start_test(C100000_RigidBody_EnablingGravityWorksPoC)
