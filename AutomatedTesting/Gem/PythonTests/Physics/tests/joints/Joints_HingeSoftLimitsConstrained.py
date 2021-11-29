"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

# Test case ID : C18243584
# Test Case Title : Check that hinge joint allows soft limit constraints on 2 bodies


# fmt: off
class Tests:
    enter_game_mode                 = ("Entered game mode",                         "Failed to enter game mode")
    exit_game_mode                  = ("Exited game mode",                          "Couldn't exit game mode")
    lead_found                      = ("Found lead",                                "Did not find lead")
    follower_found                  = ("Found follower",                            "Did not find follower")
    check_lead_position             = ("Lead stays still",                          "Lead moved")
    check_follower_position         = ("Follower moved higher than lead, but does not swing over it",        "Follower did not move higher than lead, or swinged over it")
# fmt: on


def Joints_HingeSoftLimitsConstrained():
    """
    Summary: Check that hinge joint allows soft limit constraints on 2 bodies

    Level Description:
    lead - Starts above follower entity
    follower - Starts below lead entity. Constrained to lead entity with a hinge joint. Starts with initial velocity of (5, 1, 0).
    forceRegion - This force region has a suction (negative point) force that will hold the follower if it reaches the position that will pass the test.

    Expected Behavior: 
    Lead entity remains still.
    Follower moved higher than lead, but does not swing over it. 
    Since the 45 degree limit is soft, the follower can swing to a position higher than the lead, but will not swing over it.

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
    helper.open_level("Physics", "Joints_HingeSoftLimitsConstrained")

    # 2) Enter Game Mode
    helper.enter_game_mode(Tests.enter_game_mode)

    # 3) Create and Validate Entities
    lead = Entity("lead")
    follower = Entity("follower")
    Report.info_vector3(lead.position, "lead initial position:")
    Report.info_vector3(follower.position, "follower initial position:")
    leadInitialPosition = lead.position
        
    # 4) Wait for the follower to move above the lead or Timeout
    normalizedStartPos = JointsHelper.getRelativeVector(lead.position, follower.position)
    normalizedStartPos = normalizedStartPos.GetNormalizedSafe()

    class WaitCondition:
        TARGET_ANGLE = math.radians(45)
        TARGET_MAX_ANGLE = math.radians(180)

        angleAchieved = 0.0
        followerMovedAbove45Deg = False #this is expected to be true to pass the test
        followerMovedAbove180Deg = True #this is expected to be false to pass the test            

        def checkConditionMet(self):
            #calculate the current follower-lead vector
            normalVec = JointsHelper.getRelativeVector(lead.position, follower.position)
            normalVec = normalVec.GetNormalizedSafe()
            #dot product + acos to get the angle
            currentAngle = math.acos(normalizedStartPos.Dot(normalVec))
            #if the angle is now less then last time, it is no longer rising, so end the test.
            if currentAngle < self.angleAchieved:
                return True
            
            self.angleAchieved = currentAngle
            self.followerMovedAbove45Deg = currentAngle > self.TARGET_ANGLE
            self.followerMovedAbove180Deg = currentAngle > self.TARGET_MAX_ANGLE
            return False

        def isFollowerPositionCorrect(self):
            return self.followerMovedAbove45Deg and not self.followerMovedAbove180Deg

    waitCondition = WaitCondition()

    MAX_WAIT_TIME = 5.0 #seconds
    conditionMet = helper.wait_for_condition(lambda: waitCondition.checkConditionMet(), MAX_WAIT_TIME)
    
    # 5) Check to see if lead and follower behaved as expected
    Report.info_vector3(lead.position, "lead position after test:")
    Report.info_vector3(follower.position, "follower position after test:")

    leadPositionDelta = lead.position.Subtract(leadInitialPosition)
    leadRemainedStill = JointsHelper.vector3SmallerThanScalar(leadPositionDelta, FLOAT_EPSILON)
    Report.critical_result(Tests.check_lead_position, leadRemainedStill)

    Report.critical_result(Tests.check_follower_position, conditionMet and waitCondition.isFollowerPositionCorrect())

    # 6) Exit Game Mode
    helper.exit_game_mode(Tests.exit_game_mode)



if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(Joints_HingeSoftLimitsConstrained)
