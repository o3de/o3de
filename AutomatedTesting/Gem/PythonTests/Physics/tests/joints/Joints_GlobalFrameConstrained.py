"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

# Test case ID : C18243593
# Test Case Title : Check that fixed/hinge/ball joints allow constraints to global frame


# fmt: off
class Tests:
    enter_game_mode                 = ("Entered game mode",                            "Failed to enter game mode")
    exit_game_mode                  = ("Exited game mode",                          "Couldn't exit game mode")
    follower_fixed_found            = ("Found follower for fixed joint",            "Did not find follower for fixed joint")
    follower_hinge_found            = ("Found follower for hinge joint",            "Did not find follower for hinge joint")
    follower_ball_found             = ("Found follower for ball joint",             "Did not find follower for ball joint")
    check_fixed_follower_position   = ("Fixed joint follower remained still", "Fixed joint follower did not remain still")
    check_hinge_follower_position   = ("Hinge joint follower moved in X and Z directions only", "Hinge joint follower did not move in X and Z directions, or moved in Y direction")
    check_ball_follower_position    = ("Ball joint follower moved in X, Y and Z directions", "Ball joint follower did not move in X, Y and Z directions")
# fmt: on


def Joints_GlobalFrameConstrained():
    """
    Summary: Check that fixed/hinge/ball joints allow constraints to global frame

    Level Description:
    follower_fixed - Constrained to fixed joint at global frame placed above the entity.
    follower_hinge - Constrained to hinge joint at global frame placed above the entity.
    follower_ball - Constrained to ball joint at global frame placed above the entity.

    Expected Behavior: 
    The follower-fixed entity should remain still.
    The follower_hinge and follower_ball entities move in the positive X and Z directions.

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
    FLOAT_EPSILON = 0.1

    # Helper Entity class
    class Entity(JointEntity):
        def criticalEntityFound(self): # Override function to use local Test dictionary
            Report.critical_result(Tests.__dict__[self.name + "_found"], self.id.isValid())

    # Main Script
    helper.init_idle()

    # 1) Open Level
    helper.open_level("Physics", "Joints_GlobalFrameConstrained")

    # 2) Enter Game Mode
    helper.enter_game_mode(Tests.enter_game_mode)

    # 3) Create and Validate Entities
    followerFixed = Entity("follower_fixed")
    followerHinge = Entity("follower_hinge")
    followerBall = Entity("follower_ball")
    Report.info_vector3(followerFixed.position, "follower_fixed initial position:")
    Report.info_vector3(followerHinge.position, "follower_hinge initial position:")
    Report.info_vector3(followerBall.position, "follower_ball initial position:")
    followerFixedInitialPosition = followerFixed.position
    followerHingeInitialPosition = followerHinge.position
    followerBallInitialPosition = followerBall.position

    # 4) Wait for several seconds
    general.idle_wait(1.0) # wait for lead and follower to move

    # 5) Check to see if lead and follower behaved as expected
    Report.info_vector3(followerFixed.position, "follower_fixed initial position after 1 second:")
    Report.info_vector3(followerHinge.position, "follower_hinge initial position after 1 second:")
    Report.info_vector3(followerBall.position, "follower_ball initial position after 1 second:")

    followerFixedPositionDelta = followerFixed.position.Subtract(followerFixedInitialPosition)
    fixedFollowerRemainedStill = JointsHelper.vector3SmallerThanScalar(followerFixedPositionDelta, FLOAT_EPSILON)
    Report.critical_result(Tests.check_fixed_follower_position, fixedFollowerRemainedStill)

    hingeFollowerMovedInXAndZOnly = ((followerHinge.position.x - followerHingeInitialPosition.x) > FLOAT_EPSILON and 
                        (followerHinge.position.y - followerHingeInitialPosition.y) < FLOAT_EPSILON and 
                        (followerHinge.position.z - followerHingeInitialPosition.z) > FLOAT_EPSILON)
    Report.critical_result(Tests.check_hinge_follower_position, hingeFollowerMovedInXAndZOnly)

    followerBallPositinDelta = followerBall.position.Subtract(followerBallInitialPosition)
    ballFollowerMovedinXYZ = JointsHelper.vector3LargerThanScalar(followerBallPositinDelta, FLOAT_EPSILON)
    Report.critical_result(Tests.check_ball_follower_position, ballFollowerMovedinXYZ)

    # 6) Exit Game Mode
    helper.exit_game_mode(Tests.exit_game_mode)


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(Joints_GlobalFrameConstrained)
