"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
"""

# Test case ID : 18243589
# Test Case Title : Check that ball joint allows soft limit constraints
# URL of the test case : https://testrail.agscollab.com/index.php?/cases/view/18243589

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


def C18243589_Joints_BallSoftLimitsConstrained():
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

    import ImportPathHelper as imports

    imports.init()

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
    helper.open_level("Physics", "C18243589_Joints_BallSoftLimitsConstrained")

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
    targetAngleDeg = 45
    targetAngle = math.radians(targetAngleDeg)

    maxTotalWaitTime = 2.0 #seconds
    waitTime = 0.0
    waitStep = 0.2
    angleAchieved = 0.0
    while waitTime < maxTotalWaitTime:
        waitTime = waitTime + waitStep
        general.idle_wait(waitStep) # wait for lead and follower to move

        #calculate the current follower-lead vector
        normalVec = JointsHelper.getRelativeVector(lead.position, follower.position)
        normalVec = normalVec.GetNormalizedSafe()
        #dot product + acos to get the angle
        angleAchieved = math.acos(normalizedStartPos.Dot(normalVec))
        #is it above target?
        if angleAchieved > targetAngle:
            followerMovedAboveJoint = True
            break

    # 5) Check to see if lead and follower behaved as expected
    Report.info_vector3(lead.position, "lead position after {:.2f} second:".format(waitTime))
    Report.info_vector3(follower.position, "follower position after {:.2f} second:".format(waitTime))
    angleAchievedDeg = math.degrees(angleAchieved)
    Report.info("Angle achieved {:.2f} Target {:.2f}".format(angleAchievedDeg, targetAngleDeg))

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
    import ImportPathHelper as imports
    imports.init()

    from editor_python_test_tools.utils import Report
    Report.start_test(C18243589_Joints_BallSoftLimitsConstrained)
