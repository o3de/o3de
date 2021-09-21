"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


# Test Case ID    : C4925580
# Test Case Title : Verify that Material can be assigned to Ragdoll Bones and they behave as per their material



# fmt: off
class Tests:
    enter_game_mode                      = ("Entered game mode",                                   "Failed to enter game mode")
    terrain_found_valid                  = ("PhysX Terrain found and validated",                   "PhysX Terrain not found and validated")
    concrete_ragdoll_found_valid         = ("Concrete Ragdoll found and validated",                "Concrete Ragdoll not found and validated")
    rubber_ragdoll_found_valid           = ("Rubber Ragdoll found and validated",                  "Rubber Ragdoll not found and validated")
    concrete_ragdoll_above_terrain       = ("Concrete Ragdoll is above terrain",                   "Concrete Ragdoll is not above terrain")
    rubber_ragdoll_above_terrain         = ("Rubber Ragdoll is above terrain",                     "Rubber Ragdoll is not above terrain")
    terrain_collision_detected           = ("Collision was detected on a ragdoll with terrain",    "Collision detection timed out")
    concrete_ragdoll_contacted_terrain   = ("Concrete Ragdoll contacted terrain",                  "Concrete Ragdoll did not contact terrain")
    rubber_ragdoll_contacted_terrain     = ("Rubber Ragdoll contacted terrain",                    "Rubber Ragdoll did not contact terrain")
    rubber_ragdoll_bounced_higher        = ("Rubber Ragdoll bounced higher than Concrete Ragdoll", "Rubber Ragdoll did not bounce higher than Concrete Ragdoll")
    concrete_ragdoll_bounced_as_expected = ("Concrete ragdoll bounced to expected height",         "Concrete ragdoll did not bounce to expected height")
    rubber_ragdoll_bounced_as_expected   = ("Rubber ragdoll bounced to expected height",           "Rubber ragdoll did not bounce to expected height")
    exit_game_mode                       = ("Exited game mode",                                    "Failed to exit game mode")
# fmt: on


def Material_RagdollBones():
    """
    Summary:
    This script runs an automated test to verify that assigning material to the skeleton of an actor entity with PhysX
    ragdoll will cause the entity to behave according to the nature of the material.

    Level Description:
    Two ragdoll entities (entity: Concrete Ragdoll) and (entity: Rubber Ragdoll) are above a PhysX terrain (entity:
    PhysX Terrain). Each ragdoll has an actor, an animation graph, and a PhysX ragdoll component. Gravity is enabled for
    each joint which is present on the ragdolls. The ragdolls are identical except for their textures, skeleton
    materials, and x-positions. Concrete Ragdoll's texture is blue, while Rubber Ragdoll's texture is red. Concrete
    Ragdoll's skeleton material is concrete, while Rubber Ragdoll's skeleton material is rubber.

    Expected behavior:
    The ragdolls will fall and hit the terrain at the same time. The rubber ragdoll will bounce higher than the concrete
    ragdoll.

    Test Steps:
    1) Open level and enter game mode
    2) Retrieve and validate entities
    3) Check that each ragdoll is above the terrain
    4) Wait for the initial collision between a ragdoll and the terrain or timeout
    5) Check for the maximum bounce height of each ragdoll for a given period of time
    6) Verify that the rubber ragdoll bounced higher than the concrete ragdoll
    7) Verify that each ragdoll bounced approximately to its expected maximum height
    8) Exit game mode and close editor

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
    import azlmbr.components
    import azlmbr.physics
    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper as helper

    # Constants
    TIME_OUT_SECONDS = 3.0
    TERRAIN_START_Z = 32.0
    CONCRETE_EXPECTED_MAX_BOUNCE_HEIGHT = 0.039
    RUBBER_EXPECTED_MAX_BOUNCE_HEIGHT = 1.2
    TOLERANCE = 0.5

    class Entity:
        def __init__(self, name, found_valid_test):
            self.name = name
            self.id = general.find_game_entity(name)
            self.found_valid_test = found_valid_test

    class Ragdoll(Entity):
        def __init__(self, name, found_valid_test, target_terrain, above_terrain_test, contacted_terrain_test):
            Entity.__init__(self, name, found_valid_test)
            self.target_terrain = target_terrain
            self.above_terrain_test = above_terrain_test
            self.contacted_terrain_test = contacted_terrain_test
            self.contacted_terrain = False
            self.max_bounce_height = 0
            self.reached_max_bounce = False

            # Set up collision notification handler
            self.handler = azlmbr.physics.CollisionNotificationBusHandler()
            self.handler.connect(self.id)
            self.handler.add_callback("OnCollisionBegin", self.on_collision_begin)

        def get_z_position(self):
            z_position = azlmbr.components.TransformBus(azlmbr.bus.Event, "GetWorldZ", self.id)
            return z_position

        # Set up collision detection with the terrain
        def on_collision_begin(self, args):
            other_id = args[0]
            if other_id.Equal(self.target_terrain.id):
                Report.info("{} collision began with {}".format(self.name, self.target_terrain.name))
                if not self.contacted_terrain:
                    self.hit_terrain_z = self.get_z_position()
                    self.contacted_terrain = True

    # 1) Open level and enter game mode
    helper.init_idle()
    helper.open_level("Physics", "Material_RagdollBones")
    helper.enter_game_mode(Tests.enter_game_mode)

    # 2) Retrieve and validate entities
    terrain = Entity("PhysX Terrain", Tests.terrain_found_valid)
    Report.critical_result(terrain.found_valid_test, terrain.id.IsValid())

    concrete_ragdoll = Ragdoll(
        "Concrete Ragdoll",
        Tests.concrete_ragdoll_found_valid,
        terrain,
        Tests.concrete_ragdoll_above_terrain,
        Tests.concrete_ragdoll_contacted_terrain,
    )

    rubber_ragdoll = Ragdoll(
        "Rubber Ragdoll",
        Tests.rubber_ragdoll_found_valid,
        terrain,
        Tests.rubber_ragdoll_above_terrain,
        Tests.rubber_ragdoll_contacted_terrain,
    )

    ragdolls = [concrete_ragdoll, rubber_ragdoll]
    for ragdoll in ragdolls:
        Report.critical_result(ragdoll.found_valid_test, ragdoll.id.IsValid())

        # 3) Check that each ragdoll is above the terrain
        Report.critical_result(ragdoll.above_terrain_test, ragdoll.get_z_position() > TERRAIN_START_Z)

    # 4) Wait for the initial collision between the ragdolls and the terrain or timeout
    terrain_collision_detected = helper.wait_for_condition(
        lambda: concrete_ragdoll.contacted_terrain and rubber_ragdoll.contacted_terrain, TIME_OUT_SECONDS
    )
    Report.critical_result(Tests.terrain_collision_detected, terrain_collision_detected)
    for ragdoll in ragdolls:
        Report.result(ragdoll.contacted_terrain_test, ragdoll.contacted_terrain)

    # 5) Check for the maximum bounce height of each ragdoll for a given period of time
    def check_for_max_bounce_heights(ragdolls):
        for ragdoll in ragdolls:
            if ragdoll.contacted_terrain:
                bounce_height = ragdoll.get_z_position() - ragdoll.hit_terrain_z
                if bounce_height >= ragdoll.max_bounce_height:
                    ragdoll.max_bounce_height = bounce_height
                elif ragdoll.max_bounce_height > 0.0:
                    ragdoll.reached_max_bounce = True
        return concrete_ragdoll.reached_max_bounce and rubber_ragdoll.reached_max_bounce

    helper.wait_for_condition(lambda: check_for_max_bounce_heights(ragdolls), TIME_OUT_SECONDS)
    for ragdoll in ragdolls:
        Report.info("{}'s maximum bounce height: {}".format(ragdoll.name, ragdoll.max_bounce_height))

    # 6) Verify that the rubber ragdoll bounced higher than the concrete ragdoll
    Report.result(
        Tests.rubber_ragdoll_bounced_higher, rubber_ragdoll.max_bounce_height > concrete_ragdoll.max_bounce_height
    )

    # 7) Verify that each ragdoll bounced approximately to its expected maximum height
    Report.result(
        Tests.concrete_ragdoll_bounced_as_expected,
        abs(concrete_ragdoll.max_bounce_height - CONCRETE_EXPECTED_MAX_BOUNCE_HEIGHT) < TOLERANCE,
    )
    Report.result(
        Tests.rubber_ragdoll_bounced_as_expected,
        abs(rubber_ragdoll.max_bounce_height - RUBBER_EXPECTED_MAX_BOUNCE_HEIGHT) < TOLERANCE,
    )

    # 8) Exit game mode and close editor
    helper.exit_game_mode(Tests.exit_game_mode)


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(Material_RagdollBones)
