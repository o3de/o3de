"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

# Test case ID : 18243589
# Test Case Title : Check that ball joint allows soft limit constraints


# fmt: off
class Tests:
    enter_game_mode                 = ("Entered game mode",                            "Failed to enter game mode")
    exit_game_mode                  = ("Exited game mode",                          "Couldn't exit game mode")
    lead_found                      = ("Found lead",                                "Did not find lead")
    follower_found                  = ("Found follower",                            "Did not find follower")
    check_lead_position             = ("Lead stays still",                          "Lead moved")
    check_follower_position         = ("Follower in X, Y and Z directions",         "Follower did not move in X, Y, and Z directions")
    check_follower_above_joint      = ("Follower swings above joint",               "Follower did not swing above joint")
# fmt: on


def Joints_BallSoftLimitsConstrained():
    """
    Summary: Check that ball joint allows soft limit constraints

    Level Description:
    lead - Starts above follower entity
    follower - Starts below lead entity. Constrained to lead entity with a ball joint. Starts with initial velocity of (5, 1, 0).

    Expected Behavior: 
    Lead entity remains still.
    Follower entity moves in the positive X, Y and Z directions.
    Follower entity's Z position exceeds its original Z position + 2.5, above the position where the joint is located.
    Because the cone limit is 45 degrees, if the follower manages to swing above the joint position, it is evident that the limit is soft.

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
    import math



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
    helper.open_level("Physics", "Joints_BallSoftLimitsConstrained")

    # 2) Enter Game Mode
    helper.enter_game_mode(Tests.enter_game_mode)

    # 3) Create and Validate Entities
    lead = Entity("lead")
    follower = Entity("follower")
    Report.info_vector3(lead.position, "lead initial position:")
    Report.info_vector3(follower.position, "follower initial position:")
    leadInitialPosition = lead.position
    followerInitialPosition = follower.position
    
    followerMovedAboveJoint = False
    #calculate  the start vector from follower and lead positions
    normalizedStartPos = JointsHelper.getRelativeVector(lead.position, follower.position)
    normalizedStartPos = normalizedStartPos.GetNormalizedSafe()
    #the targeted angle to reach between the initial vector and the current follower-lead vector
    TARGET_ANGLE_DEG = 45
    targetAngle = math.radians(TARGET_ANGLE_DEG)
    angleAchieved = 0.0

    def checkAngleMet():
        #calculate the current follower-lead vector
        normalVec = JointsHelper.getRelativeVector(lead.position, follower.position)
        normalVec = normalVec.GetNormalizedSafe()
        #dot product + acos to get the angle
        angleAchieved = math.acos(normalizedStartPos.Dot(normalVec))
        #is it above target?
        return angleAchieved > targetAngle

    MAX_WAIT_TIME = 2.0 #seconds
    followerMovedAboveJoint = helper.wait_for_condition(checkAngleMet, MAX_WAIT_TIME)

    # 5) Check to see if lead and follower behaved as expected
    Report.info_vector3(lead.position, "lead position:")
    Report.info_vector3(follower.position, "follower position:")
    angleAchievedDeg = math.degrees(angleAchieved)
    Report.info(f"Angle achieved {angleAchievedDeg:.2f} Target {TARGET_ANGLE_DEG:.2f}")

    leadPositionDelta = lead.position.Subtract(leadInitialPosition)
    leadRemainedStill = JointsHelper.vector3SmallerThanScalar(leadPositionDelta, FLOAT_EPSILON)
    Report.critical_result(Tests.check_lead_position, leadRemainedStill)

    followerPositionDelta = follower.position.Subtract(followerInitialPosition)
    followerMovedinXYZ = JointsHelper.vector3LargerThanScalar(followerPositionDelta, FLOAT_EPSILON)
    Report.critical_result(Tests.check_follower_position, followerMovedinXYZ)

    Report.critical_result(Tests.check_follower_above_joint, followerMovedAboveJoint)

    # 6) Exit Game Mode
    helper.exit_game_mode(Tests.exit_game_mode)


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(Joints_BallSoftLimitsConstrained)
