"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

# Test case ID : C18243582
# Test Case Title : Check that fixed joint allows lead-follower collision


# fmt: off
class Tests:
    enter_game_mode                 = ("Entered game mode",               "Failed to enter game mode")
    exit_game_mode                  = ("Exited game mode",                "Couldn't exit game mode")
    lead_found                      = ("Found lead",                      "Did not find lead")
    follower_found                  = ("Found follower",                  "Did not find follower")
    check_collision_happened        = ("Lead and follower collided",      "Lead and follower did not collide")
# fmt: on


def C18243582_Joints_FixedLeadFollowerCollide():
    """
    Summary: Check that fixed joint allows lead-follower collision

    Level Description:
    lead - Starts above follower entity
    follower - Starts below lead entity. Constrained to lead entity with fixed joint. Starts with initial velocity of (5, 0, 0) in positive X direction.

    Expected Behavior: 
    The follower entity moves in the positive X direction and the lead entity is dragged along towards the positive X direction.
    The x position of the lead entity is incremented from its original.
    The lead and follower entities are kept apart at a distance of approximately 1.0 due to collision.

    Test Steps:
    1) Open Level
    2) Enter Game Mode
    3) Create and Validate Entities
    4) Wait for several seconds
    5) Check to see if lead and follower behaved as expected.
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

    import ImportPathHelper as imports

    imports.init()

    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper as helper

    import azlmbr.legacy.general as general
    import azlmbr.bus

    from JointsHelper import JointEntityCollisionAware

    # Helper Entity class - self.collided flag is set when instance receives collision event.
    class Entity(JointEntityCollisionAware):
        def criticalEntityFound(self): # Override function to use local Test dictionary
            Report.critical_result(Tests.__dict__[self.name + "_found"], self.id.isValid())

    # Main Script
    helper.init_idle()

    # 1) Open Level
    helper.open_level("Physics", "C18243582_Joints_FixedLeadFollowerCollide")

    # 2) Enter Game Mode
    helper.enter_game_mode(Tests.enter_game_mode)

    # 3) Create and Validate Entities
    lead = Entity("lead")
    follower = Entity("follower")

    # 4) Wait for several seconds
    general.idle_wait(2.0) # wait for lead and follower to move

    # 5) Check to see if lead entity and follower collided
    Report.critical_result(Tests.check_collision_happened, lead.collided and follower.collided)

    # 6) Exit Game Mode
    helper.exit_game_mode(Tests.exit_game_mode)



if __name__ == "__main__":
    import ImportPathHelper as imports
    imports.init()

    from editor_python_test_tools.utils import Report
    Report.start_test(C18243582_Joints_FixedLeadFollowerCollide)
