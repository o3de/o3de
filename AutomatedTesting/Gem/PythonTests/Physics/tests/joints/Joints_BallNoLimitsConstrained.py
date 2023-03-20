"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

# Test case ID : C18243590
# Test Case Title : Check that ball joint allows no limit constraints


# fmt: off
class Tests:
    enter_game_mode                 = ("Entered game mode",                            "Failed to enter game mode")
    exit_game_mode                  = ("Exited game mode",                          "Couldn't exit game mode")
    lead_found                      = ("Found lead",                                "Did not find lead")
    follower_found                  = ("Found follower",                            "Did not find follower")
    check_lead_position             = ("Lead stays still",                          "Lead moved")
    check_follower_position         = ("Follower moved higher than lead",           "Follower did not move higher than lead")
# fmt: on


def Joints_BallNoLimitsConstrained():
    """
    Summary: Check that ball joint allows no limit constraints

    Level Description:
    lead - Starts above follower entity
    follower - Starts below lead entity. Constrained to lead entity with a ball joint. The ball joint is located in between the lead and follower. Starts with initial velocity of (5, 1, 0).
    dampingRegion - A force region cube is placed at the position of the lead. If the follower swings to the position above and near the lead, the force region's damping holds the follower in the place.'

    Expected Behavior: 
    Lead entity remains still.
    Follower entity's Z position exceeds lead entity's Z position.
    Because the ball joint is somewhere in the middle of the follower and the lead, 
    if the follower manages to go above the lead, 
    it is evident that the ball joint without limits managed to keep the follower constrained to the lead but did not impose a limit.

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

    import JointsHelper
    from JointsHelper import JointEntity

    # Constants
    FLOAT_EPSILON = 0.2

    # Helper Entity class
    class Entity(JointEntity):
        def criticalEntityFound(self): # Override function to use local Test dictionary
            Report.critical_result(Tests.__dict__[self.name + "_found"], self.id.isValid())

    # Main Script
    helper.init_idle()

    # 1) Open Level
    helper.open_level("Physics", "Joints_BallNoLimitsConstrained")

    # 2) Enter Game Mode
    helper.enter_game_mode(Tests.enter_game_mode)

    # 3) Create and Validate Entities
    lead = Entity("lead")
    follower = Entity("follower")
    Report.info_vector3(lead.position, "lead initial position:")
    Report.info_vector3(follower.position, "follower initial position:")
    leadInitialPosition = lead.position
    followerInitialPosition = follower.position

    # 4) Wait for several seconds
    general.idle_wait(3.0) # wait for lead and follower to move

    # 5) Check to see if lead and follower behaved as expected
    Report.info_vector3(lead.position, "lead position after 2.5 second:")
    Report.info_vector3(follower.position, "follower position after 2.5 second:")

    leadPositionDelta = lead.position.Subtract(leadInitialPosition)
    leadRemainedStill = JointsHelper.vector3SmallerThanScalar(leadPositionDelta, FLOAT_EPSILON)
    Report.critical_result(Tests.check_lead_position, leadRemainedStill)

    followerAboveLead = follower.position.z > leadInitialPosition.z
    Report.critical_result(Tests.check_follower_position, followerAboveLead)

    # 6) Exit Game Mode
    helper.exit_game_mode(Tests.exit_game_mode)



if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(Joints_BallNoLimitsConstrained)
