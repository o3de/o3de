"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


# Test case ID : C5340400
# Test Case Title : Verify that when Compute inertia is disabled, the user gets to set the moment of inertia
#                   and physX engine work accordingly



# fmt: off
class Tests():
    enter_game_mode                 = ("Entered game mode",                     "Failed to enter game mode")
    find_upper_box                  = ("Upper Box found",                       "Upper Box not found")
    find_lower_box                  = ("Lower Box found",                       "Lower Box not found")
    find_physx_terrain              = ("PhysX Terrain found",                   "PhysX Terrain not found")
    boxes_collided                  = ("Boxes collided",                        "Boxes did not collide")
    upper_box_did_not_topple_t1     = ("Upper Box did not topple at time t1",   "Upper Box toppled at time t1")
    upper_box_did_not_touch_ground  = ("Upper Box did not touch the terrain",   "Upper Box touched the terrain")
    upper_box_did_not_topple_t2     = ("Upper Box did not topple at time t2",   "Upper Box toppled at time t2")
    exit_game_mode                  = ("Exited game mode",                      "Failed to exit game mode")
# fmt: on


def RigidBody_MomentOfInertiaManualSetting():
    """
    Summary:
    Runs an automated test to ensure that assigning a high moment of inertia component causes the entity to exhibit
    high intertia along the component axis and therefore resist change in motion along that axis.

    Level Description:
    A box (entity: Upper Box) set above and askew to another box (entity: Lower Box) which is set on a PhysX terrain
    (entity: PhysX Terrain).
    Gravity is enabled.
    Lower box has computed inertia.
    Upper box has Inertia Diagonal x=10000, y=1, z=1.

    Expected behavior:
    The upper box falls onto the lower box and tilts extremely slowly toward the terrain. The torque exerted on the
    upper box by the opposing influences of momentum, normal force from the lower box, and/or ongoing acceleration due
    to gravity will be greatly resisted by the Inertia Diagonal x=10000 resulting in a tiny change in angular velocity.

    Test Steps:
    1) Open level
    2) Enter game mode
    3) Retrieve entities
    4) Check for collision between the two boxes
        4.5) Wait for the boxes to collide
    5) Check that the upper box's x-angular velocity is insignificant at time t1 upon collision
    6) Check that the upper box does not collide with the terrain in a given time
        6.5) Wait for the given time
    7) Check that the upper box's x-angular velocity remains insignificant at time t2 after collision
    8) Exit game mode
    9) Close editor

    Note:
    - This test file must be called from the Open 3D Engine Editor command terminal
    - Any passed and failed tests are written to the Editor.log file.
            Parsing the file or running a log_monitor are required to observe the test results.

    :return: None
    """
    # Setup path
    import os
    import sys
    import azlmbr.legacy.general as general
    import azlmbr.bus
    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper as helper

    # Specific wait time in seconds
    TIME_OUT = 3.0

    # X-angular velocity should be positive and tiny
    def XAngularVelocityIsValid(x_ang_vel):
        TOLERANCE = 0.01
        return 0 < x_ang_vel < TOLERANCE

    helper.init_idle()

    # 1) Open level
    helper.open_level("Physics", "RigidBody_MomentOfInertiaManualSetting")

    # 2) Enter game mode
    helper.enter_game_mode(Tests.enter_game_mode)

    # 3) Retrieve entities
    upper_box_id = general.find_game_entity("Upper Box")
    Report.result(Tests.find_upper_box, upper_box_id.IsValid())

    lower_box_id = general.find_game_entity("Lower Box")
    Report.result(Tests.find_lower_box, lower_box_id.IsValid())

    physx_terrain_id = general.find_game_entity("PhysX Terrain")
    Report.result(Tests.find_physx_terrain, physx_terrain_id.IsValid())

    # 4) Check for collision between the two boxes
    class BoxesCollided:
        value = False

    def on_boxes_collision_begin(args):
        other_id = args[0]
        if other_id.Equal(lower_box_id):
            Report.info("Boxes collided")
            BoxesCollided.value = True

    handler = azlmbr.physics.CollisionNotificationBusHandler()
    handler.connect(upper_box_id)
    handler.add_callback("OnCollisionBegin", on_boxes_collision_begin)

    # 4.5) Wait for the boxes to collide
    helper.wait_for_condition(lambda: BoxesCollided.value, TIME_OUT)
    Report.result(Tests.boxes_collided, BoxesCollided.value)

    # 5) Check that the upper box's x-angular velocity is insignificant at time t1 upon collision
    # The torque on the upper box caused by its momentum and the normal force from the lower box is resisted at time t1
    upper_box_angular_velocity = azlmbr.physics.RigidBodyRequestBus(
        azlmbr.bus.Event, "GetAngularVelocity", upper_box_id
    )
    Report.info("Upper Box's x-angular velocity at time t1 upon collision: {}".format(upper_box_angular_velocity.x))
    Report.result(Tests.upper_box_did_not_topple_t1, XAngularVelocityIsValid(upper_box_angular_velocity.x))

    # 6) Check that the upper box does not collide with the terrain in a given time
    # This will also validate that the angular velocity does not reach a local maximum
    class UpperBoxCollidedWithTerrain:
        value = False

    def on_ground_collision_begin(args):
        other_id = args[0]
        if other_id.Equal(physx_terrain_id):
            Report.info("Upper Box collided with terrain")
            UpperBoxCollidedWithTerrain.value = True

    handler.add_callback("OnCollisionBegin", on_ground_collision_begin)

    # 6.5) Wait for the given time
    helper.wait_for_condition(lambda: UpperBoxCollidedWithTerrain.value, TIME_OUT)
    Report.result(Tests.upper_box_did_not_touch_ground, not UpperBoxCollidedWithTerrain.value)

    # 7) Check that the upper box's x-angular velocity remains insignificant at time t2 after collision
    # The torque on the upper box caused by gravity and the normal force from the lower box is resisted through time t2
    upper_box_angular_velocity = azlmbr.physics.RigidBodyRequestBus(
        azlmbr.bus.Event, "GetAngularVelocity", upper_box_id
    )
    Report.info("Upper Box's x-angular velocity at time t2 after collision: {}".format(upper_box_angular_velocity.x))
    Report.result(Tests.upper_box_did_not_topple_t2, XAngularVelocityIsValid(upper_box_angular_velocity.x))

    # 8) Exit game mode
    helper.exit_game_mode(Tests.exit_game_mode)



if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(RigidBody_MomentOfInertiaManualSetting)
