"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.
SPDX-License-Identifier: Apache-2.0 OR MIT
"""


class Tests:
    enter_game_mode = (
        "Entered Game Mode",
        "Failed to enter Game Mode"
    )
    glass_cube_moves = (
        "Glass cube started moving",
        "Glass cube has unexpectedly remained stationary"
    )
    cubes_are_stationary = (
        "Cubes have both come to a rest",
        "One or both cubes are unexpectedly still moving"
    )
    exit_game_mode = (
        "Exited Game Mode",
        "Failed to exit Game Mode"
    )


def TerrainPhysicsCollider_MaterialMapping_Works():
    """
    Summary:
    Opens an existing level with a Terrain Layer Spawner split into 2 different surface areas with 2 different Physics
    Materials applied. Test verifies that physicalized objects properly interact with the specified Physics Material
    mapped surfaces on terrain.
    :return: None
    """

    import azlmbr.legacy.general as general
    import azlmbr.bus as bus
    import azlmbr.components as components

    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper as helper

    def vector_is_close_to_zero(vector):
        return abs(vector.x) <= 0.001 and abs(vector.y) <= 0.001 and abs(vector.z) <= 0.001

    def is_stationary(entity):
        velocity = azlmbr.physics.RigidBodyRequestBus(bus.Event, "GetLinearVelocity", entity)
        return vector_is_close_to_zero(velocity)

    # Open a level with preconfigured terrain with surfaces mapped to Physics Materials
    helper.init_idle()
    helper.open_level("Terrain", "TerrainPhysicsCollider_MaterialMapping_Works")

    # Enter Game Mode
    helper.enter_game_mode(Tests.enter_game_mode)

    # Find the needed entities
    physics_cube_glass = general.find_game_entity("PhysicsCube_Glass")
    physics_cube_rubber = general.find_game_entity("PhysicsCube_Rubber")

    # Get the initial positions of each Physics Cube
    glass_start = components.TransformBus(bus.Event, "GetWorldTranslation", physics_cube_glass)
    rubber_start = components.TransformBus(bus.Event, "GetWorldTranslation", physics_cube_rubber)

    # Wait for cube on glass to start moving, then wait for both cubes to become stationary, re-check positions, and
    # calculate distance traveled
    glass_is_moving = helper.wait_for_condition(lambda: not is_stationary(physics_cube_glass), 3.0)
    Report.result(Tests.glass_cube_moves, glass_is_moving)
    glass_is_stationary = helper.wait_for_condition(lambda: is_stationary(physics_cube_glass), 30.0)
    rubber_is_stationary = helper.wait_for_condition(lambda: is_stationary(physics_cube_rubber), 10.0)
    Report.result(Tests.cubes_are_stationary, glass_is_stationary and rubber_is_stationary)
    glass_finish = components.TransformBus(bus.Event, "GetWorldTranslation", physics_cube_glass)
    rubber_finish = components.TransformBus(bus.Event, "GetWorldTranslation", physics_cube_rubber)
    glass_distance = glass_finish.GetDistance(glass_start)
    rubber_distance = rubber_finish.GetDistance(rubber_start)

    # Report results on distance traveled and validate that the Cube on the glass surface travels further
    expected_glass_distance = 10.0
    expected_rubber_distance = 1.0
    material_interaction = (
        f"PhysicsCube_Glass moved {glass_distance:.3f}m, to PhysicsCube_Rubber's {rubber_distance:.3f}m",
        f"PhysicsCube_Rubber: Moved {rubber_distance:.3f}m, expected < {expected_rubber_distance}m. "
        f"PhysicsCube_Glass: Moved {glass_distance:.3f}m, expected > {expected_glass_distance}m"
    )
    Report.result(material_interaction, glass_distance > expected_glass_distance and
                  rubber_distance < expected_rubber_distance)

    # Exit Game Mode
    helper.exit_game_mode(Tests.exit_game_mode)


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(TerrainPhysicsCollider_MaterialMapping_Works)
