"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

# Test Case ID    : C15308221
# Test Case Title : Verify that material library and slots are always in sync and work consistently through the different places of usage



# fmt: off
class Tests:
    enter_game_mode_0           = ("Test 0) Entered game mode",                       "Test 0) Failed to enter game mode")
    find_terrain_0              = ("Test 0) The Terrain was found",                   "Test 0) The Terrain was not found")
    find_terrain_box_0          = ("Test 0) Terrain test box was found",              "Test 0) Terrain test box was not found")
    find_collider_0             = ("Test 0) Box collider was found",                  "Test 0) Box collider was not found")
    find_ragdoll_0              = ("Test 0) Ragdoll was found",                       "Test 0) Ragdoll was not found")
    find_character_controller_0 = ("Test 0) Character controller was found",          "Test 0) Character controller was not found")
    find_controller_box_0       = ("Test 0) Character controller test box was found", "Test 0) Character controller test box was not found")

    terrain_box_bounced_0       = ("Test 0) Terrain test box bounced",                "Test 0) Terrain test box did not bounce")
    collider_bounced_0          = ("Test 0) Box collider bounced",                    "Test 0) Box collider did not bounce")
    ragdoll_bounced_0           = ("Test 0) Modified ragdoll bounced",                "Test 0) Modified ragdoll did not bounce")
    controller_box_bounced_0    = ("Test 0) Character controller test box bounced",   "Test 0) Character controller test box did not bounce")
    exit_game_mode_0            = ("Test 0) Exited game mode",                        "Test 0) Failed to exit game mode")
    all_bounced_equal_0         = ("Test 0) All entities bounced the same height",    "Test 0) All entities did not bounce the same height")

    enter_game_mode_1           = ("Test 1) Entered game mode",                       "Test 1) Failed to enter game mode")
    find_terrain_1              = ("Test 1) The Terrain was found",                   "Test 1) The Terrain was not found")
    find_terrain_box_1          = ("Test 1) Terrain test box was found",              "Test 1) Terrain test box was not found")
    find_collider_1             = ("Test 1) Box collider was found",                  "Test 1) Box collider was not found")
    find_ragdoll_1              = ("Test 1) Ragdoll was found",                       "Test 1) Ragdoll was not found")
    find_character_controller_1 = ("Test 1) Character controller was found",          "Test 1) Character controller was not found")
    find_controller_box_1       = ("Test 1) Character controller test box was found", "Test 1) Character controller test box was not found")

    terrain_box_bounced_1       = ("Test 1) Terrain test box bounced",                "Test 1) Terrain test box did not bounce")
    collider_bounced_1          = ("Test 1) Box collider bounced",                    "Test 1) Box collider did not bounce")
    ragdoll_bounced_1           = ("Test 1) Modified ragdoll bounced",                "Test 1) Modified ragdoll did not bounce")
    controller_box_bounced_1    = ("Test 1) Character controller test box bounced",   "Test 1) Character controller test box did not bounce")
    exit_game_mode_1            = ("Test 1) Exited game mode",                        "Test 1) Failed to exit game mode")
    all_bounced_equal_1         = ("Test 1) All entities bounced the same height",    "Test 1) All entities did not bounce the same height")

    all_bounced_greater         = ("All entities bounced higher on the second test",  "All entities did not bounce higher on the second test")
# fmt: on


def Material_ComponentsInSyncWithLibrary():
    """
    Summary:
    Runs an automated test to verify that the material library is always in sync between the different PhysX components

    Level Description:
    A new material library was created with 1 material, called "Modified":
        dynamic friction: 0.5
        static friction:  0.5
        restitution:      0.25

    There are 4 types of components we want to test for:
        PhysX Ragdoll:
            A ragdoll ("ragdoll") with the "Modified" material applied to all of its colliders. Positioned above the
            terrain.
        PhysX collider:
            A PhysX box collider ("collider") with a the "Modified" material applied. Positioned above the terrain.
        PhysX terrain:
            A PhysX terrain ("terrain"), and a PhysX box collider ("terrain_box"). "terrain_box" is positioned above
            "terrain". A new layer was created with the "Modified" material and painted onto the terrain under
            "terrain_box". "terrain_box" has the default material applied.
        PhysX character controller:
            A character controller ("character_controller"), and a PhysX box collider ("controller_box").
            "controller_box" is positioned above "character_controller" and is assigned the default material.
            "character_controller" is assigned "Modified"

    Expected behavior:
    For every iteration this test measures the bounce height of each entity. The entities save their traveled distances
    each iteration, to verify different behavior between each setup.

    First the test verifies the entities all behave identically, without changing anything. All entities should bounce
    the same height.

    Next, the test modifies the restitution value for 'Modified' (from 0.25 to 0.75). All entities should again bounce
    the same height. Additionally, all entities should bounce higher with the new restitution than they did previously.

    Test Steps:
    1) Open level
    2) Collect basis values without modifying anything
        2.1) Enter game mode
        2.2) Find entities
        2.3) Wait for entities to bounce
        2.4) Exit game mode
    3) Verify all entities behave the same as a baseline
    4) Modify the restitution value of 'modified'
        4.1 - 4.4) <same as 2.1 - 2.4>
    5) Verify the entities all still behave the same
    6) Verify that the material change was propagated correctly
    7) Close editor

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

    from Physmaterial_Editor import Physmaterial_Editor
    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper as helper

    TIMEOUT = 3.0
    BOUNCE_TOLERANCE = 0.1

    class Entity:
        def __init__(self, name, bounce_off_of_name):
            self.name = name
            self.bounce_off_of_name = bounce_off_of_name
            self.bounces = []

        def find_and_reset(self):
            self.hit_position = None
            self.hit_terrain = False
            self.max_bounce = 0.0
            self.reached_max_bounce = False
            self.id = general.find_game_entity(self.name)
            self.setup_handler()
            return self.id.IsValid()

        def on_collision_enter(self, args):
            entering = args[0]
            if entering.Equal(self.id):
                if not self.hit_terrain:
                    self.hit_terrain_position = self.position
                    self.hit_terrain = True

        def setup_handler(self):
            self.bounce_off_of_id = general.find_game_entity(self.bounce_off_of_name)
            self.handler = azlmbr.physics.CollisionNotificationBusHandler()
            self.handler.connect(self.bounce_off_of_id)
            self.handler.add_callback("OnCollisionBegin", self.on_collision_enter)

        @property
        def position(self):
            return azlmbr.components.TransformBus(bus.Event, "GetWorldTranslation", self.id)

    def get_test(test_name):
        return Tests.__dict__[test_name]

    def run_test(test_number):
        # x.1) Enter game mode
        helper.enter_game_mode(get_test("enter_game_mode_{}".format(test_number)))

        # x.2) Find entities
        controller_valid = general.find_game_entity("character_controller").IsValid()
        terrain_valid = general.find_game_entity("terrain").IsValid()

        Report.critical_result(get_test("find_character_controller_{}".format(test_number)), controller_valid)
        Report.critical_result(get_test("find_terrain_{}".format(test_number)), terrain_valid)

        collider_valid = collider.find_and_reset()
        controller_box_valid = controller_box.find_and_reset()
        ragdoll_valid = ragdoll.find_and_reset()
        terrain_box_valid = terrain_box.find_and_reset()

        Report.critical_result(get_test("find_collider_{}".format(test_number)), collider_valid)
        Report.critical_result(get_test("find_controller_box_{}".format(test_number)), controller_box_valid)
        Report.critical_result(get_test("find_ragdoll_{}".format(test_number)), ragdoll_valid)
        Report.critical_result(get_test("find_terrain_box_{}".format(test_number)), terrain_box_valid)

        def wait_for_bounce():
            for entity in all_entities:
                if entity.hit_terrain:
                    current_bounce_height = entity.position.z - entity.hit_terrain_position.z
                    if current_bounce_height >= entity.max_bounce:
                        entity.max_bounce = current_bounce_height
                    elif entity.max_bounce > 0.0:
                        entity.reached_max_bounce = True
            return all([entity.reached_max_bounce for entity in all_entities])

        # x.3) Wait for entities to bounce
        helper.wait_for_condition(wait_for_bounce, TIMEOUT)

        Report.result(get_test("collider_bounced_{}".format(test_number)), collider.reached_max_bounce)
        Report.result(get_test("controller_box_bounced_{}".format(test_number)), controller_box.reached_max_bounce)
        Report.result(get_test("ragdoll_bounced_{}".format(test_number)), ragdoll.reached_max_bounce)
        Report.result(get_test("terrain_box_bounced_{}".format(test_number)), terrain_box.reached_max_bounce)

        for entity in all_entities:
            entity.bounces.append(entity.max_bounce)

        # x.4) Exit game mode
        helper.exit_game_mode(get_test("exit_game_mode_{}".format(test_number)))

    # 1) Open level and enter game mode
    helper.init_idle()
    helper.open_level("Physics", "Material_ComponentsInSyncWithLibrary")

    # Setup persisting entities
    collider = Entity("collider", "terrain")
    controller_box = Entity("controller_box", "character_controller")
    ragdoll = Entity("ragdoll", "terrain")
    terrain_box = Entity("terrain_box", "terrain")
    all_entities = [collider, controller_box, ragdoll, terrain_box]

    # 2) Collect basis values without modifying anything
    run_test(0)

    # 3) Verify all entities behave the same as a baseline
    test_0_max_bounce = max([entity.bounces[0] for entity in all_entities])
    test_0_min_bounce = min([entity.bounces[0] for entity in all_entities])
    Report.result(
        Tests.all_bounced_equal_0, lymath.Math_IsClose(test_0_max_bounce, test_0_min_bounce, BOUNCE_TOLERANCE)
    )

    # 4) Modify the restitution value of 'modified'
    material_editor = Physmaterial_Editor("c15308221_material_componentsinsyncwithlibrary.physmaterial")
    material_editor.modify_material("Modified", "Restitution", 0.75)
    material_editor.save_changes()
    run_test(1)

    # 5) Verify the entities all still behave the same
    test_1_max_bounce = max([entity.bounces[1] for entity in all_entities])
    test_1_min_bounce = min([entity.bounces[1] for entity in all_entities])
    Report.result(
        Tests.all_bounced_equal_1, lymath.Math_IsClose(test_1_max_bounce, test_1_min_bounce, BOUNCE_TOLERANCE)
    )

    # 6) Verify that the material change was propagated correctly
    all_bounced_greater = all([entity.bounces[0] < entity.bounces[1] for entity in all_entities])
    Report.result(Tests.all_bounced_greater, all_bounced_greater)



if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(Material_ComponentsInSyncWithLibrary)
