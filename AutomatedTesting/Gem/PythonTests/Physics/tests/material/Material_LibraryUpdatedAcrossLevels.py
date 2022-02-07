"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

# Test case ID : C15425935
# Test Case Title : Verify that the change in Material Library gets updated across levels


# fmt: off
class Tests:
    # Game Mode 0
    enter_game_mode_0         = ("Entered game mode 0",                    "Failed to enter game mode 0")
    modify_sphere_0_found     = ("Test 0: modify_sphere was found",        "Test 0: modify_sphere was not found")
    delete_sphere_0_found     = ("Test 0: delete_sphere was found",        "Test 0: delete_sphere was not found")
    terrain_0_found           = ("Test 0: terrain Entity found",           "Test 0: terrain Entity was not found")
    trigger_0_found           = ("Test 0: trigger entity found",           "Test 0: trigger entity wasn't found")
    sphere_initial_position_0 = ("Test 0: spheres initial position valid", "Test 0: spheres initial position not valid")
    sphere_initial_velocity_0 = ("Test 0: spheres initial velocity valid", "Test 0: spheres initial velocity not valid")
    sphere_collision_0        = ("Test 0: Both spheres collided",          "Test 0: Both spheres did not collide")
    exit_game_mode_0          = ("Exited game mode 0",                     "Couldn't exit game mode 0")
    # Game Mode 1
    enter_game_mode_1         = ("Entered game mode 1",                    "Failed to enter game mode 1")
    modify_sphere_1_found     = ("Test 1: modify_sphere was found",        "Test 1: modify_sphere was not found")
    delete_sphere_1_found     = ("Test 1: delete_sphere was found",        "Test 1: delete_sphere was not found")
    terrain_1_found           = ("Test 1: terrain Entity found",           "Test 1: terrain Entity was not found")
    trigger_1_found           = ("Test 1: trigger entity found",           "Test 1: trigger entity wasn't found")
    sphere_initial_position_1 = ("Test 1: spheres initial position valid", "Test 1: spheres initial position not valid")
    sphere_initial_velocity_1 = ("Test 1: spheres initial velocity valid", "Test 1: spheres initial velocity not valid")
    sphere_collision_1        = ("Test 1: Both spheres collided",          "Test 1: Both spheres did not collide")
    exit_game_mode_1          = ("Exited game mode 1",                     "Couldn't exit game mode 1")
    # Game Mode 2
    enter_game_mode_2         = ("Entered game mode 2",                    "Failed to enter game mode 2")
    modify_sphere_2_found     = ("Test 2: modify_sphere was found",        "Test 2: modify_sphere was not found")
    delete_sphere_2_found     = ("Test 2: delete_sphere was found",        "Test 2: delete_sphere was not found")
    terrain_2_found           = ("Test 2: terrain Entity found",           "Test 2: terrain Entity was not found")
    trigger_2_found           = ("Test 2: trigger entity found",           "Test 2: trigger entity wasn't found")
    sphere_initial_position_2 = ("Test 2: spheres initial position valid", "Test 2: spheres initial position not valid")
    sphere_initial_velocity_2 = ("Test 2: spheres initial velocity valid", "Test 2: spheres initial velocity not valid")
    sphere_collision_2        = ("Test 2: Both spheres collided",          "Test 2: Both spheres did not collide")
    exit_game_mode_2          = ("Exited game mode 2",                     "Couldn't exit game mode 2")
    # Game Mode 3
    enter_game_mode_3         = ("Entered game mode 3",                    "Failed to enter game mode 3")
    modify_sphere_3_found     = ("Test 3: modify_sphere was found",        "Test 3: modify_sphere was not found")
    delete_sphere_3_found     = ("Test 3: delete_sphere was found",        "Test 3: delete_sphere was not found")
    terrain_3_found           = ("Test 3: terrain Entity found",           "Test 3: terrain Entity was not found")
    trigger_3_found           = ("Test 3: trigger entity found",           "Test 3: trigger entity wasn't found")
    sphere_initial_position_3 = ("Test 3: spheres initial position valid", "Test 3: spheres initial position not valid")
    sphere_initial_velocity_3 = ("Test 3: spheres initial velocity valid", "Test 3: spheres initial velocity not valid")
    sphere_collision_3        = ("Test 3: Both spheres collided",          "Test 3: Both spheres did not collide")
    exit_game_mode_3          = ("Test 3: Exited game mode 3",             "Couldn't exit game mode 3")

    # Test Verification
    baseline_verified         = ("Both levels are the same",               "Both levels aren't the same")
    material_delete_verified  = ("Material delete updated spheres",        "Material delete not updated spheres")
    material_modify_verified  = ("Material modify updated spheres",        "Material modify not updated spheres")
    post_change_verified      = ("Both levels are still the same",         "Both levels are not the same")

# fmt: on


def Material_LibraryUpdatedAcrossLevels():
    """
    Summary: Verify that the change in a physmaterial library gets updated across levels

    Level Description: There are two levels that are being compared. Each are exact replicas with a shared
        material library
    modify_sphere - Starts between terrain and trigger with initial velocity in negative z direction and gravity disabled;
        has physx collider in sphere shape with material "to_delete", had physx rigid body, and sphere_shape
    delete_sphere - Starts between terrain and trigger with initial velocity in negative z direction and gravity disabled;
        has physx collider in sphere shape with material "to_modify", had physx rigid body, and sphere_shape
    terrain - Default terrain with transform inline; has physx terrain component
    trigger - Above the spheres, trigger is enabled; has physx collider in box shape with dimensions (5.0, 10.0, 0.25)
        and box shape with the same dimensions

    Expected Behavior: Materials deleted or modified have their changes update across levels. Initially the spheres will
        not bounce off the terrain after the change to the material library they will bounce up and hit the trigger

    Material Tests:
    Test 0 - Tests level 0 before the material change
    Test 1 - Tests level 1 before the material change
    Test 2 - Tests level 0 after the material change
    Test 3 - Tests level 1 after the material change

    Test 0 and 1 should be exactly the same. Test 2 and 3 should be exactly the same. Both modification to the material
    library should allow the spheres to bounce in Test 2 and 3. Therefore, both spheres will have a higher velocity and
    be able to trigger in Test 2 and 3 as compared to 0 and 1.

    Iterated Game Mode steps:
    1) Open the correct level for the test
    2) Open Game Mode
    3) Create and Verify Entities
    4) Wait for Sphere collision with Terrain Entity
    5) Wait for spheres to have a chance to hit trigger
    6) Modify Material Library
    7) Exit Game Mode

    Test Steps:
    1) Create Test Objects
    2) Run Game Mode steps once for each test
    3) Verify that spheres acted as expected
    4) Close Editor

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
    import azlmbr.math as math

    from Physmaterial_Editor import Physmaterial_Editor

    # Constants
    FLOAT_THRESHOLD = sys.float_info.epsilon
    TIMEOUT = 1
    INITIAL_VELOCITY = math.Vector3(0.0, 0.0, -10.0)
    VELOCITY_THRESHOLD = 0.1

    # Helper Functions
    class Entity:
        def __init__(self, name, index):
            self.id = general.find_game_entity(name)
            self.name = name
            self.collision_happened = False
            self.index = index
            # ID validation
            self.found = Tests.__dict__[self.name + "_{}_found".format(index)]
            Report.critical_result(self.found, self.id.IsValid())

        @property
        def position(self):
            return azlmbr.components.TransformBus(azlmbr.bus.Event, "GetWorldTranslation", self.id)

        @property
        def velocity(self):
            return azlmbr.physics.RigidBodyRequestBus(azlmbr.bus.Event, "GetLinearVelocity", self.id)

    class Sphere(Entity):
        terrain_id = None

        def __init__(self, name, index):
            Entity.__init__(self, name, index)
            # Set Handler
            self.handler = azlmbr.physics.CollisionNotificationBusHandler()
            self.handler.connect(self.id)
            self.handler.add_callback("OnCollisionBegin", self.on_collision_begin)

        def on_collision_begin(self, args):
            if args[0].equal(Sphere.terrain_id):
                self.collision_happened = True


    class Material_Test:
        def __init__(self, index, level_index):
            # index is the test index 0-3 this allows for tests from the Tests class to be fetched
            self.index = index
            # level_index determins which level will be opened at the start of the test loop
            self.level_index = level_index
            # Data
            self.modify_sphere_hit_trigger = False
            self.delete_sphere_hit_trigger = False
            self.entity_list = None
            self.modify_sphere_final_velocity = None
            self.delete_sphere_final_velocity = None

        def sphere_initial_position(self, modify_sphere_position, delete_sphere_position, terrain_position, trigger_position):
            position_valid = (
                modify_sphere_position.z == delete_sphere_position.z
                and modify_sphere_position.z > terrain_position.z
                and trigger_position.z > modify_sphere_position.z
            )
            initial_position = Tests.__dict__["sphere_initial_position_{}".format(self.index)]
            Report.critical_result(initial_position, position_valid)

        def sphere_initial_velocity(self, modify_sphere_velocity, delete_sphere_velocity):
            initial_velocity_string = Tests.__dict__["sphere_initial_velocity_{}".format(self.index)]
            Report.critical_result(
                initial_velocity_string,
                modify_sphere_velocity.IsClose(INITIAL_VELOCITY, VELOCITY_THRESHOLD) and delete_sphere_velocity.IsClose(INITIAL_VELOCITY, VELOCITY_THRESHOLD),
            )

        def log_velocity(self):
            self.modify_sphere_final_velocity = self.entity_list[0].velocity
            self.delete_sphere_final_velocity = self.entity_list[1].velocity

        def set_trigger(self):
            # Type (Entity) -> None
            # Sets handler for trigger
            self.handler = azlmbr.physics.TriggerNotificationBusHandler()
            self.handler.connect(self.entity_list[3].id)
            self.handler.add_callback("OnTriggerEnter", self.on_trigger_enter)

        def on_trigger_enter(self, args):
            # Type () -> None
            # When trigger entered the correct sphere is found boolean is flipped
            if self.entity_list[0].id.Equal(args[0]):
                self.modify_sphere_hit_trigger = True
            if self.entity_list[1].id.Equal(args[0]):
                self.delete_sphere_hit_trigger = True

    def are_levels_consistent(level_a, level_b):
        triggers_0 = level_a.modify_sphere_hit_trigger == level_b.modify_sphere_hit_trigger
        triggers_1 = level_a.delete_sphere_hit_trigger == level_b.delete_sphere_hit_trigger
        modify_sphere_velocities = (
            abs(level_a.modify_sphere_final_velocity.z - level_b.modify_sphere_final_velocity.z) < FLOAT_THRESHOLD
        )
        delete_sphere_velocities = (
            abs(level_a.delete_sphere_final_velocity.z - level_b.delete_sphere_final_velocity.z) < FLOAT_THRESHOLD
        )
        return triggers_0 and triggers_1 and modify_sphere_velocities and delete_sphere_velocities

    def check_material_delete(test_0, test_3):
        triggers = test_0.modify_sphere_hit_trigger != test_3.modify_sphere_hit_trigger
        modify_sphere_velocities = test_0.modify_sphere_final_velocity.z < test_3.modify_sphere_final_velocity.z
        return triggers and modify_sphere_velocities

    def check_material_modify(test_0, test_3):
        triggers = test_0.delete_sphere_hit_trigger != test_3.delete_sphere_hit_trigger
        delete_sphere_velocities = test_0.delete_sphere_final_velocity.z < test_3.delete_sphere_final_velocity.z
        return triggers and delete_sphere_velocities

    def modify_material_library():
        physmaterial_object = Physmaterial_Editor("Material_LibraryUpdatedAcrossLevels.physmaterial")
        physmaterial_object.delete_material("to_delete")
        physmaterial_object.modify_material("to_modify", "Restitution", 1.0)
        physmaterial_object.save_changes()

    helper.init_idle()
    # 1) Create Test Objects
    # Each test object is given an index that will determine what tuples are pulled from the Tests class and are indicative of the order that they will be run.
    # Each test object also has a level_index to determine which level will be opened during the test loop. Both levels 0 and 1 are looked at before and after
    # the change to the material library
    test_0 = Material_Test(index=0, level_index=0)
    test_1 = Material_Test(index=1, level_index=1)
    test_2 = Material_Test(index=2, level_index=0)
    test_3 = Material_Test(index=3, level_index=1)
    # Test list of all the tests in order of index
    test_list = [test_0, test_1, test_2, test_3]

    # 2) Run Game Mode steps once for each test
    for test in test_list:
        # 1) Open the correct level for the test
        helper.open_level(
            "physics",
            "Material_LibraryUpdatedAcrossLevels\\Material_LibraryUpdatedAcrossLevels_{}".format(
                test.level_index
            ),
        )

        # 2) Open Game Mode
        helper.enter_game_mode(Tests.__dict__["enter_game_mode_{}".format(test.index)])

        # 3) Create and Verify Entities
        terrain = Entity("terrain", test.index)
        Sphere.terrain_id = terrain.id
        modify_sphere = Sphere("modify_sphere", test.index)
        delete_sphere = Sphere("delete_sphere", test.index)
        trigger = Entity("trigger", test.index)
        test.entity_list = [modify_sphere, delete_sphere, terrain, trigger]
        test.set_trigger()

        test.sphere_initial_position(modify_sphere.position, delete_sphere.position, terrain.position, trigger.position)
        test.sphere_initial_velocity(modify_sphere.velocity, delete_sphere.velocity)

        # 4) Wait for Sphere collision with Terrain Entity
        collisions_happened = helper.wait_for_condition(lambda: modify_sphere.collision_happened and delete_sphere.collision_happened, TIMEOUT) 
        Report.result(Tests.__dict__["sphere_collision_{}".format(test.index)], collisions_happened)

        # 5) Wait for spheres to have a chance to hit trigger
        helper.wait_for_condition(lambda: test.modify_sphere_hit_trigger and test.delete_sphere_hit_trigger, TIMEOUT)
        # Report trigger
        Report.info("modify_sphere{} hit trigger in test {}".format("" if test.modify_sphere_hit_trigger else " didn't", test.index))
        Report.info("delete_sphere{} hit trigger in test {}".format("" if test.delete_sphere_hit_trigger else " didn't", test.index))

        test.log_velocity()

        # 6) Modify Material Library
        if test.index == 1:
            modify_material_library()

        # 7) Exit Game Mode
        helper.exit_game_mode(Tests.__dict__["exit_game_mode_{}".format(test.index)])

    # 3) Verify that spheres acted as expected
    Report.result(Tests.baseline_verified, are_levels_consistent(test_0, test_1))
    Report.result(Tests.material_delete_verified, check_material_delete(test_0, test_3))
    Report.result(Tests.material_modify_verified, check_material_modify(test_0, test_3))
    Report.result(Tests.post_change_verified, are_levels_consistent(test_2, test_3))



if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(Material_LibraryUpdatedAcrossLevels)
