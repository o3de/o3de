"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

# Test case ID : C18243581
# Test Case Title : Check that fixed joint is breakable


# fmt: off
class Tests:
    enter_game_mode                 = ("Entered game mode",               "Failed to enter game mode")
    exit_game_mode                  = ("Exited game mode",                "Couldn't exit game mode")
    lead_found                      = ("Found lead",                      "Did not find lead")
    follower_found                  = ("Found follower",                  "Did not find follower")
    check_lead_position             = ("Lead did not move in X direction","Lead moved in X direction")
    check_follower_position         = ("Follower moved in X direction",   "Follower did not move in X direction")
# fmt: on


def Joints_FixedBreakable():
    """
    Summary: Check that fixed joint is breakable

    Level Description:
    lead - Starts above follower entity
    follower - Starts below lead entity. Constrained to lead entity with fixed joint. Starts with initial velocity of (5, 0, 0) in positive X direction.

    Expected Behavior: The follower entity moves in the positive X direction but the lead entity does not move in the positive X direction by more than a distance of 0.5.

    Test Steps:
    1) Open Level
    2) Enter Game Mode
    3) Create and Validate Entities
    4) Wait for several seconds
    5) Check to see if lead and follower behaved as expected
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

    import azlmbr.legacy.general as general
    import azlmbr.bus

    from JointsHelper import JointEntity

    # Helper Entity class
    class Entity(JointEntity):
        def criticalEntityFound(self): # Override function to use local Test dictionary
            Report.critical_result(Tests.__dict__[self.name + "_found"], self.id.isValid())

    # Main Script
    helper.init_idle()

    # 1) Open Level
    helper.open_level("Physics", "Joints_FixedBreakable")

    # 2) Enter Game Mode
    helper.enter_game_mode(Tests.enter_game_mode)

    # 3) Create and Validate Entities
    lead = Entity("lead")
    follower = Entity("follower")
    Report.info_vector3(lead.position, "lead initial position:")
    Report.info_vector3(follower.position, "follower initial position:")
    leadInitialPosition = lead.position.x
    followerInitialPosition = follower.position.x

    # 4) Wait for several seconds
    general.idle_wait(1.0) # wait for lead and follower to move

    # 5) Check to see if lead and follower behaved as expected
    Report.info_vector3(lead.position, "lead position after 1 second:")
    Report.info_vector3(follower.position, "follower position after 1 second:")
    Report.critical_result(Tests.check_lead_position, (lead.position.x - leadInitialPosition) < 0.5)
    Report.critical_result(Tests.check_follower_position, (follower.position.x - followerInitialPosition) > 0.5)

    # 6) Exit Game Mode
    helper.exit_game_mode(Tests.exit_game_mode)



if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(Joints_FixedBreakable)
