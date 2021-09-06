"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

# Test Case ID    : C4044694
# Test Case Title : Verify that if we add an empty Material library in Collider Component, the object continues to use Default material values



# fmt: off
class Tests:
    enter_game_mode             = ("Entered game mode",                                             "Failed to enter game mode")
    find_terrain                = ("The Terrain was found",                                         "The Terrain was not found")
    find_default_box            = ("'default_box' was found",                                       "'default_box' was not found")
    find_empty_box              = ("'empty_box' was found",                                         "'empty_box' was not found")
    find_default_sphere         = ("'default_sphere' was found",                                    "'default_sphere' was not found")
    find_empty_sphere           = ("'empty_sphere' was found",                                      "'empty_sphere' was not found")
    boxes_moved                 = ("All boxes moved",                                               "Boxes failed to move")
    boxes_at_rest               = ("All boxes came to rest",                                        "Boxes failed to come to rest")
    default_sphere_bounced      = ("'default_sphere' bounced",                                      "'default_sphere' did not bounce")
    empty_sphere_bounced        = ("'empty_sphere' bounced",                                        "'empty_sphere' did not bounce")
    default_box_equals_empty    = ("'default_box' and 'empty_box' traveled the same distance",      "'default_box' and 'empty_box' did not travel the same distance")
    default_sphere_equals_empty = ("'default_sphere' and 'empty_sphere' bounce heights were equal", "'default_sphere' and 'empty_sphere' bounce heights were not equal")
    exit_game_mode              = ("Exited game mode",                                              "Failed to exit game mode")
# fmt: on


def Material_EmptyLibraryUsesDefault():
    """
    Summary:
    Runs an automated test to verify that an object with an empty Material library in a Collider Component continues to
    use the default material values

    Level Description:
    There are 5 entities.
        One terrain entity ('terrain') with PhysX Terrain,
        Two sphere entities ('empty_sphere' and 'default_sphere') with PhysX Rigid Body and PhysX Sphere Collider,
        Two box entities ('empty_box' and 'default_box') with PhysX Rigid Body and PhysX Box Collider,

    The spheres are positioned above the terrain, and the boxes are placed on the terrain.
    The "empty" entities are assigned a material library that contains no materials. The "default" entities are assigned
    the default material from the default material library.

    Expected behavior:
    The spheres fall and bounce the same height.
    The boxes are pushed and travel the same distance.

    Test Steps:
    1) Open level and enter game mode
    2) Find entities
    3) Wait for spheres to bounce
    4) Compare 'default_sphere' to 'empty_sphere'
    5) Push the boxes and wait for them to come to rest
    6) Compare 'default_box' to 'empty_box'
    7) Exit game mode and close editor

    Note:
    - This test file must be called from the Open 3D Engine Editor command terminal
    - Any passed and failed tests are written to the Editor.log file.
            Parsing the file or running a log_monitor are required to observe the test results.

    :return: None
    """
    import os
    import sys
    import azlmbr.legacy.general as general
    import azlmbr.bus as bus
    import azlmbr.components
    import azlmbr.physics
    import azlmbr.math as lymath

    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper as helper

    FORCE_IMPULSE = lymath.Vector3(5.0, 0.0, 0.0)
    TIMEOUT = 3.0
    DISTANCE_TOLERANCE = 0.001

    class Entity:
        def __init__(self, name):
            self.name = name
            self.id = general.find_game_entity(self.name)

        @property
        def position(self):
            return azlmbr.components.TransformBus(bus.Event, "GetWorldTranslation", self.id)

    class Box(Entity):
        def __init__(self, name):
            Entity.__init__(self, name)
            self.start_position = self.position

        def is_stationary(self):
            velocity = azlmbr.physics.RigidBodyRequestBus(bus.Event, "GetLinearVelocity", self.id)
            return velocity.IsZero()

        def push(self):
            azlmbr.physics.RigidBodyRequestBus(bus.Event, "ApplyLinearImpulse", self.id, FORCE_IMPULSE)

    class Sphere(Entity):
        def __init__(self, name):
            Entity.__init__(self, name)
            self.hit_terrain_position = None
            self.hit_terrain = False
            self.max_bounce = 0.0
            self.reached_max_bounce = False

    def on_collision_enter(args):
        entering = args[0]
        for sphere in [default_sphere, empty_sphere]:
            if sphere.id.Equal(entering):
                if not sphere.hit_terrain:
                    sphere.hit_terrain_position = sphere.position
                    sphere.hit_terrain = True

    # region wait_for_condition() functions
    def wait_for_bounce():
        for sphere in [default_sphere, empty_sphere]:
            if sphere.hit_terrain:
                current_bounce_height = sphere.position.z - sphere.hit_terrain_position.z
                if current_bounce_height >= sphere.max_bounce:
                    sphere.max_bounce = current_bounce_height
                elif sphere.max_bounce > 0.0:
                    sphere.reached_max_bounce = True
        return default_sphere.reached_max_bounce and empty_sphere.reached_max_bounce

    def boxes_moved():
        return not default_box.is_stationary() and not empty_box.is_stationary()

    def boxes_are_stationary():
        return default_box.is_stationary() and empty_box.is_stationary()

    # endregion

    # 1) Open level and enter game mode
    helper.init_idle()
    helper.open_level("Physics", "Material_EmptyLibraryUsesDefault")
    helper.enter_game_mode(Tests.enter_game_mode)

    # 2) Find entities
    terrain_id = general.find_game_entity("terrain")
    default_box = Box("default_box")
    empty_box = Box("empty_box")
    default_sphere = Sphere("default_sphere")
    empty_sphere = Sphere("empty_sphere")

    Report.result(Tests.find_terrain, terrain_id.IsValid())
    Report.result(Tests.find_default_box, default_box.id.IsValid())
    Report.result(Tests.find_empty_box, empty_box.id.IsValid())
    Report.result(Tests.find_default_sphere, default_sphere.id.IsValid())
    Report.result(Tests.find_empty_sphere, empty_sphere.id.IsValid())

    # Setup terrain collision handler
    handler = azlmbr.physics.CollisionNotificationBusHandler()
    handler.connect(terrain_id)
    handler.add_callback("OnCollisionBegin", on_collision_enter)

    # 3) Wait for spheres to bounce
    helper.wait_for_condition(wait_for_bounce, TIMEOUT)
    Report.result(Tests.default_sphere_bounced, default_sphere.reached_max_bounce)
    Report.result(Tests.empty_sphere_bounced, empty_sphere.reached_max_bounce)

    # 4) Compare 'default_sphere' to 'empty_sphere'
    sphere_bounces_equal = lymath.Math_IsClose(default_sphere.max_bounce, empty_sphere.max_bounce, DISTANCE_TOLERANCE)
    Report.result(Tests.default_sphere_equals_empty, sphere_bounces_equal)

    # 5) Push the boxes and wait for them to come to rest
    default_box.push()
    empty_box.push()
    Report.result(Tests.boxes_moved, helper.wait_for_condition(boxes_moved, TIMEOUT))
    Report.result(Tests.boxes_at_rest, helper.wait_for_condition(boxes_are_stationary, TIMEOUT))

    # 6) Compare 'default_box' to 'empty_box'
    default_distance = default_box.position.GetDistance(default_box.start_position)
    empty_distance = empty_box.position.GetDistance(empty_box.start_position)
    box_distances_equal = lymath.Math_IsClose(default_distance, empty_distance, DISTANCE_TOLERANCE)
    Report.result(Tests.default_box_equals_empty, box_distances_equal)

    # 7) Exit game mode and close editor
    helper.exit_game_mode(Tests.exit_game_mode)


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(Material_EmptyLibraryUsesDefault)
