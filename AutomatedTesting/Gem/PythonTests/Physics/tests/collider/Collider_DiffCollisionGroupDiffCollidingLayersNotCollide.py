"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


# Test case ID : C4982593
# Test Case Title : Check that two entities with different collision groups and layers do not collide.



# fmt: off
class Tests:
    enter_game_mode         = ("Entered game mode",                             "Failed to enter game mode")
    moving_entity_found     = ("Moving entity found",                           "Moving entity not found")
    stationary_entity_found = ("Stationary entity found",                       "Stationary entity not found")
    moving_pos_found        = ("Moving sphere position found",                  "Moving sphere position not found")
    stationary_pos_found    = ("Stationary sphere position found",              "Stationary sphere position not found")
    spheres_share_x_axis    = ("Both spheres are aligned properly",             "Spheres are not aligned properly")
    spheres_not_collided    = ("No collision was detected",                     "A collision was detected")
    spheres_switched_sides  = ("The moving sphere passed through the other",    "Moving sphere did not pass through")
    no_y_movement           = ("There was no Y movement",                       "Some Y movement was detected")
    no_z_movement           = ("There was no Z movement",                       "Some Z movement was detected")
    timed_out               = ("Test did not time out",                         "Test TIMED OUT")
    stationary_didnt_move   = ("Stationary sphere did not move",                "Stationary sphere moved")
    exit_game_mode          = ("Exited game mode",                              "Couldn't exit game mode")

# fmt: on


def Collider_DiffCollisionGroupDiffCollidingLayersNotCollide():
    # type: () -> None
    """
    Summary:
    Runs an automated test to ensure to rigid bodies on the different collision group and collision layer
    DO NOT collide.

    Level Description:
    Moving (entity) - a spherical entity (colored yellow) that is set up with a collision layer of "demo1"
        collision group of "demo_group1" gravity as disabled and an initial velocity of (3, 0, 0).
    Stationary (entity) - a spherical entity (colored purple) that is set up with a collision layer of "demo2"
        collision group of "demo_group2" gravity disabled, and is positioned at location (+2, 0, 0) relative to
        Moving's starting position.

    Expected Behavior:
    When game mode is entered, Moving will begin moving in the positive X direction. The entity should pass through
    Stationary with no collision detection triggered.

    Test Steps:
    1) Loads the level / Enter game mode
    2) Retrieve test entities
    3) Ensures that the test objects (Moving and Stationary) are located
        3.5) set up variables and handlers
    4) Wait for Moving to pass through Stationary
    5) Logs results
    6) Close the editor

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

    # **** Helper class ****

    class Sphere:
        def __init__(self, name):
            self.id = None
            self.name = name
            self.initial_pos = None
            self.current_position = None

    # Tests for significant change between a set of float pairs
    #   returns a list of booleans where the i-th index hold the result for the i-th float pair
    def detect_significant_change(float_pairs):
        # type: ([(float, float)]) -> [bool]
        return [abs(p[0] - p[1]) >= CLOSE_ENOUGH_THRESHOLD for p in float_pairs]

    # *** Executable Code ***

    # Constants
    TIME_OUT = 1.5
    CLOSE_ENOUGH_THRESHOLD = 0.0001
    SPHERE_RADIUS = 0.5  # Radius of both sphere entities

    # 1) Open level / Enter game mode
    helper.init_idle()
    helper.open_level("Physics", "Collider_DiffCollisionGroupDiffCollidingLayersNotCollide")
    helper.enter_game_mode(Tests.enter_game_mode)

    # 2) Retrieve/validate entities
    moving = Sphere("Moving")
    moving.id = general.find_game_entity(moving.name)
    Report.critical_result(Tests.moving_entity_found, moving.id.IsValid())

    stationary = Sphere("Stationary")
    stationary.id = general.find_game_entity(stationary.name)
    Report.critical_result(Tests.stationary_entity_found, stationary.id.IsValid())

    # 3) Log starting positions
    moving.initial_pos = azlmbr.components.TransformBus(azlmbr.bus.Event, "GetWorldTranslation", moving.id)
    stationary.initial_pos = azlmbr.components.TransformBus(azlmbr.bus.Event, "GetWorldTranslation", stationary.id)

    # Ensure that spheres are aligned properly along the y and z axises (so x axis movement will cause collision)
    spheres_aligned = (abs(moving.initial_pos.y - stationary.initial_pos.y) < SPHERE_RADIUS) and (
        abs(moving.initial_pos.z - stationary.initial_pos.z) < SPHERE_RADIUS
    )

    # Report critical level integrity results
    Report.critical_result(Tests.moving_pos_found, moving.initial_pos is not None and not moving.initial_pos.IsZero())
    Report.critical_result(
        Tests.stationary_pos_found, stationary.initial_pos is not None and not moving.initial_pos.IsZero()
    )
    Report.critical_result(
        Tests.spheres_share_x_axis,
        spheres_aligned,
        "Please check the level to make sure Moving Sphere and Stationary Sphere share y and z positions",
    )

    # 3.5) Set up variables and handler for observing force region interaction

    class TestData:
        collision_occurred = False
        spheres_switched = False

    # Force Region Event Handler
    def on_collision_begin(args):
        collider_id = args[0]
        if collider_id.Equal(moving.id):
            if not TestData.collision_occurred:
                TestData.collision_occurred = True
                Report.info("Collision detected")

    # Assign the handler
    handler = azlmbr.physics.CollisionNotificationBusHandler()
    handler.connect(stationary.id)
    handler.add_callback("OnCollisionBegin", on_collision_begin)

    moving.current_pos = moving.initial_pos
    stationary.current_pos = stationary.initial_pos

    # Tests if we are done collecting results and can exit the test
    def done_collecting_results():

        COMPARISON_BUFFER = 0.2

        if not TestData.collision_occurred:

            moving.current_pos = azlmbr.components.TransformBus(azlmbr.bus.Event, "GetWorldTranslation", moving.id)
            stationary.current_pos = azlmbr.components.TransformBus(azlmbr.bus.Event, "GetWorldTranslation", stationary.id)

            if moving.current_pos.x > stationary.current_pos.x + COMPARISON_BUFFER:
                # Moving sphere passed the stationary sphere's x coordinate
                TestData.spheres_switched = True
                return True
        else:
            # A collision was detected
            Report.info("A collision was detected unfortunately")
            return True

        return False

    # 4) Wait for results to be collected or for time out
    Report.result(Tests.timed_out, helper.wait_for_condition(done_collecting_results, TIME_OUT))

    # 5) Log results
    Report.result(Tests.spheres_switched_sides, TestData.spheres_switched)
    Report.result(Tests.spheres_not_collided, not TestData.collision_occurred)

    # Look for movement in Y direction. Report results
    y_movement = detect_significant_change(
        [(moving.initial_pos.y, moving.current_pos.y), (stationary.initial_pos.y, stationary.current_pos.y)]
    )

    if not any(y_movement):
        Report.success(Tests.no_y_movement)
    else:
        Report.failure(Tests.no_y_movement)
        if y_movement[0]:
            Report.info("Moving entity Y movement detected. This should not happen")
        if y_movement[1]:
            Report.info("Stationary entity Y movement detected. This should not happen")

    # Look for movement in Z direction. Report Results
    z_movement = detect_significant_change(
        [(moving.initial_pos.z, moving.current_pos.z), (stationary.initial_pos.z, stationary.current_pos.z)]
    )

    if not any(z_movement):
        Report.success(Tests.no_z_movement)
    else:
        Report.failure(Tests.no_z_movement)
        if z_movement[0]:
            Report.info("Moving entity Z movement detected. This should not happen")
        if z_movement[1]:
            Report.info("Stationary entity Z movement detected. This should not happen")

    Report.result(Tests.stationary_didnt_move, stationary.current_pos.Equal(stationary.initial_pos))

    # Collected data dump
    Report.info(" ********** Collected Data ***************")
    Report.info("Moving sphere's positions:")
    Report.info_vector3(moving.initial_pos, "  initial:")
    Report.info_vector3(moving.current_pos, "  final:")
    Report.info("*****************************")
    Report.info("Stationary sphere's positions:")
    Report.info_vector3(stationary.initial_pos, "  initial:")
    Report.info_vector3(stationary.current_pos, "  final:")
    Report.info("*****************************")

    # 6) Exit Game mode
    helper.exit_game_mode(Tests.exit_game_mode)

    Report.info("*** FINISHED TEST ***")


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(Collider_DiffCollisionGroupDiffCollidingLayersNotCollide)
