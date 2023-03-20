"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

# Test case ID : C18243591
# Test Case Title : Check that ball joint allows lead-follower collision


# fmt: off
class Tests:
    enter_game_mode                 = ("Entered game mode",                            "Failed to enter game mode")
    exit_game_mode                  = ("Exited game mode",                          "Couldn't exit game mode")
    lead_found                      = ("Found lead",                                "Did not find lead")
    follower_found                  = ("Found follower",                            "Did not find follower")
    check_collision_happened        = ("Lead and follower collided",                "Lead and follower did not collide")
# fmt: on


def Joints_BallLeadFollowerCollide():
    """
    Summary: Check that ball joint allows lead-follower collision

    Level Description:
    lead - Starts above follower entity
    follower - Starts below lead entity. Constrained to lead entity with a ball joint. Starts with initial velocity of (5, 2, 0).

    Expected Behavior: 
    Lead entity remains still.
    Follower entity swings up, collides with the lead entity, and falls back down.

    Test Steps:
    1) Open Level
    2) Enter Game Mode
    3) Create and Validate Entities
    4) Wait for several seconds
    5) Check to see if lead and follower behaved as expected (they collided)
    6) Exit Game Mode
    7) Close Editor

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

    from JointsHelper import JointEntityCollisionAware

    # Helper Entity class - self.collided flag is set when instance receives collision event.
    class Entity(JointEntityCollisionAware):
        def criticalEntityFound(self): # Override function to use local Test dictionary
            Report.critical_result(Tests.__dict__[self.name + "_found"], self.id.isValid())

    # Main Script
    helper.init_idle()

    # 1) Open Level
    helper.open_level("Physics", "Joints_BallLeadFollowerCollide")

    # 2) Enter Game Mode
    helper.enter_game_mode(Tests.enter_game_mode)

    # 3) Create and Validate Entities
    lead = Entity("lead")
    follower = Entity("follower")

    # 4) Wait for collision between lead and follower or timeout
    Report.critical_result(Tests.check_collision_happened, helper.wait_for_condition(lambda: lead.collided and follower.collided, 10.0))

    # 5) Exit Game Mode
    helper.exit_game_mode(Tests.exit_game_mode)

if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(Joints_BallLeadFollowerCollide)
