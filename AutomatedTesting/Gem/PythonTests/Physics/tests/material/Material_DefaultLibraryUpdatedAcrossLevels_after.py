"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

# Test case ID : C15096732
# Test Case Title : Verify Default material library works across different levels


# fmt: off
class Tests:
    # Game Mode 2
    enter_game_mode_2         = ("Entered game mode 2",                   "Failed to enter game mode 2")
    sphere_found_2            = ("Test 2: Sphere was found",              "Test 2: Sphere was not found")
    terrain_found_2           = ("Test 2: Terrrain Entity found",         "Test 2: Terrain Entity was not found")
    trigger_found_2           = ("Test 2: trigger found",                 "Test 2: trigger not found")
    sphere_initial_position_2 = ("Test 2: Sphere initial position valid", "Test 2: Sphere initial position not valid")
    sphere_initial_velocity_2 = ("Test 2: Sphere initial velocity valid", "Test 2: Sphere initial velocity not valid")
    sphere_collision_2        = ("Test 2: Sphere collided with Terrain",  "Test 2: Sphere did not collide")
    exit_game_mode_2          = ("Exited game mode 2",                    "Couldn't exit game mode 2")
    # Game Mode 3
    enter_game_mode_3         = ("Entered game mode 3",                   "Failed to enter game mode 3")
    sphere_found_3            = ("Test 3: Sphere was found",              "Test 3: Sphere was not found")
    terrain_found_3           = ("Test 3: Terrrain Entity found",         "Test 3: Terrain Entity was not found")
    trigger_found_3           = ("Test 3: trigger found",                 "Test 3: trigger not found")
    sphere_initial_position_3 = ("Test 3: Sphere initial position valid", "Test 3: Sphere initial position not valid")
    sphere_initial_velocity_3 = ("Test 3: Sphere initial velocity valid", "Test 3: Sphere initial velocity not valid")
    sphere_collision_3        = ("Test 3: Sphere collided with Terrain",  "Test 3: Sphere did not collide")
    exit_game_mode_3          = ("Exited game mode 3",                    "Couldn't exit game mode 3")

    # Test Verification
    levels_start_equal        = ("Both levels are the same",              "Both levels are not the same")
    material_library_switch   = ("Library switch updated the sphere",     "Library switch didn't update sphere")
    levels_stay_equal         = ("Both levels are still the same",        "Both levels are not the same post_change")
# fmt: on


def Material_DefaultLibraryUpdatedAcrossLevels_after():
    """
    Summary: Verify Default material library works across different levels, this is the second stage to the test.
        The reload was required for the editor to pick up changes in default material library in the
        default.physxconfiguration file. After the tests are run this script will load the data from the previous
        script and compare it to the two new tests to see if changing the default material library progpogated
        correctly. C15096732_Material_DefaultLibraryUpdatedAcrossLevels_b.physmaterial is the default material
        file for these two tests.

    Level Description: There are two levels indexed 1 and 2 each are completely identical to the other.
    sphere - Placed between trigger and terrain with velocity in the negative z direction; has physx rigid body,
        physx collider with sphere shape and useful_material_0 assigned (before change in default material library)
        and sphere shape
    trigger - A trigger sitting above the sphere and terrain; has physx rigid body, physx collider with box
        shape, and box shape
    terrain - Terrain placeholder with transform inline with default terrain height; has physx terrain
        component with default characteristics

    physxconfiguration files: The change in default material library is achieved by launching the editor twice and 
        overriding the default.physxconfiguration file with files that are nearly identical other than having
        different default material libraries
    
    Materials: no_bounce_0 and no_bounce_1 are set so that they do not bounce from the terrain. global
        Default and bounce_0 and bounce_1 are set so that a bounce does occur. During the test the sphere first has
        a no_bounce material applied after the change in default material library to one with the bounce material the
        sphere has the global Default material applied. The bounce materials exist to avoid any current or future
        issues with an empty material library.

    Test run explanation:
        Test 0: Collect baseline for default material library in the first level
        Test 1: Collect baseline for default material library in the second level
        Test 2: Collect resulting data for changed material library in the first level
        Test 3: Collect resulting data for changed material library in the second level

    Expected Behavior: For the two test run by this script the ball will bounce from the terrain and hit the trigger
        as the material for spheres is now the global Default material.

    Iterated Game Mode steps:
    1) Open the correct level is open
    2) Enter Game Mode
    3) Create and Verify Entities
    4) Wait for Sphere collision with Terrain Entity
    5) Allow time to hit trigger
    6) Log Final Values
    7) Exit Game Mode

    Test Steps:
    1) Create Test Objects
    2) Run Game Mode steps once for each test
    3) Read results from local tmp file
    4) Validate test wide results
    5) Close Editor

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

    # Constants
    FLOAT_THRESHOLD = 0.0001
    TIMEOUT = 2.0
    INITIAL_VELOCITY = math.Vector3(0.0, 0.0, -10.0)

    # Helper Functions
    class Entity:
        def __init__(self, name, index):
            self.id = general.find_game_entity(name)
            self.name = name
            self.index = index
            # ID validation
            self.found = Tests.__dict__["{}_found_{}".format(self.name, index)]
            Report.critical_result(self.found, self.id.IsValid())

        @property
        def position(self):
            return azlmbr.components.TransformBus(azlmbr.bus.Event, "GetWorldTranslation", self.id)

        @property
        def velocity(self):
            return azlmbr.physics.RigidBodyRequestBus(azlmbr.bus.Event, "GetLinearVelocity", self.id)

    class Material_Test:
        def __init__(self, test_index, level):
            self.test_index = test_index
            self.level = level
            self.entity_list = None
            # Setting Flags
            self.terrain_collision = False
            self.trigger_triggered = False

        def set_handlers(self):
            trigger = self.entity_list[2]
            # Set handler for collision
            self.handler_0 = azlmbr.physics.CollisionNotificationBusHandler()
            self.handler_0.connect(self.entity_list[0].id)
            self.handler_0.add_callback("OnCollisionBegin", self.on_collision_begin)
            # Set handler for trigger
            self.handler = azlmbr.physics.TriggerNotificationBusHandler()
            self.handler.connect(trigger.id)
            self.handler.add_callback("OnTriggerEnter", self.on_trigger_enter)

        def on_collision_begin(self, args):
            if args[0].equal(self.entity_list[1].id):
                self.terrain_collision = True

        def on_trigger_enter(self, args):
            if args[0].equal(self.entity_list[0].id):
                self.trigger_triggered = True

        def check_sphere_initial_position(self, position_valid):
            initial_position = Tests.__dict__["sphere_initial_position_{}".format(self.test_index)]
            Report.critical_result(initial_position, position_valid)

        def check_sphere_initial_velocity(self):
            initial_velocity_string = Tests.__dict__["sphere_initial_velocity_{}".format(self.test_index)]
            Report.critical_result(initial_velocity_string, self.entity_list[0].velocity.IsClose(INITIAL_VELOCITY, 0.1))

        def check_sphere_collision(self):
            collision = Tests.__dict__["sphere_collision_{}".format(self.test_index)]
            Report.result(collision, self.terrain_collision)

    def default_material_library_changed_as_expected(velocity_list, hit_trigger_list):
        hit_trigger_change = not hit_trigger_list[1] and hit_trigger_list[2]
        velocity_change_valid = (
            abs(velocity_list[1].x - velocity_list[2].x) < FLOAT_THRESHOLD
            and abs(velocity_list[1].y - velocity_list[2].y) < FLOAT_THRESHOLD
            and velocity_list[1].z <= velocity_list[2].z
        )

        return velocity_change_valid and hit_trigger_change

    def compare_level_baseline(velocity_list, hit_trigger_list):
        velocities_valid = (
            abs(velocity_list[0].z - velocity_list[1].z) < FLOAT_THRESHOLD
            and abs(velocity_list[0].y - velocity_list[1].y) < FLOAT_THRESHOLD
            and abs(velocity_list[0].x - velocity_list[1].x) < FLOAT_THRESHOLD
        )
        hit_trigger_correct = hit_trigger_list[0] == hit_trigger_list[1]

        return velocities_valid and hit_trigger_correct

    def levels_coinsistent_after_modification(velocity_list, hit_trigger_list):
        velocities_valid = (
            abs(velocity_list[2].z - velocity_list[3].z) < 0.01
            and abs(velocity_list[2].y - velocity_list[3].y) < FLOAT_THRESHOLD
            and abs(velocity_list[2].x - velocity_list[3].x) < FLOAT_THRESHOLD
        )
        hit_trigger_correct = hit_trigger_list[2] == hit_trigger_list[3]

        return velocities_valid and hit_trigger_correct

    def get_data_from_previous_tests():
        from ast import literal_eval

        try:
            with open(
                os.path.join(
                    os.getcwd(), "AutomatedTesting", "Levels", "Physics", "Material_DefaultLibraryUpdatedAcrossLevels", "_last_run_before_change_data.txt"
                )) as data_file:
                lines = data_file.readlines()
                for i, line in enumerate(lines):
                    if i < 2:
                        line = literal_eval(line)
                        lines[i] = math.Vector3(float(line[0]), float(line[1]), float(line[2]))
                    else:
                        lines[i] = line == "True"
        except Exception as e:
            Report.info(e)
            helper.fail_fast("Could not save data of first two tests.")
        return lines[:2], lines[2:4]

    helper.init_idle()
    # 1) Create Test Objects
    test_2 = Material_Test(test_index=2, level=0)
    test_3 = Material_Test(test_index=3, level=1)
    test_list = [test_2, test_3]

    # 2) Run Game Mode steps once for each test
    for test in test_list:
        # 1) Open the correct level is open
        helper.open_level(
            "physics",
            f"Material_DefaultLibraryUpdatedAcrossLevels\\{test.level}"
        )

        # 2) Enter Game Mode
        helper.enter_game_mode(Tests.__dict__["enter_game_mode_{}".format(test.test_index)])

        # 3) Create and Verify Entities
        sphere = Entity("sphere", test.test_index)
        terrain = Entity("terrain", test.test_index)
        trigger = Entity("trigger", test.test_index)
        test.entity_list = [sphere, terrain, trigger]

        position_valid = terrain.position.z < sphere.position.z < trigger.position.z
        test.check_sphere_initial_position(position_valid)
        test.check_sphere_initial_velocity()

        # 4) Wait for Sphere collision with Terrain Entity
        test.set_handlers()
        helper.wait_for_condition(lambda: test.terrain_collision, TIMEOUT)
        test.check_sphere_collision()

        # 5) Allow time for Sphere to hit trigger
        helper.wait_for_condition(lambda: test.trigger_triggered, TIMEOUT)

        # 6) Log Final Values
        test.final_velocity = sphere.velocity

        # 7) Exit Game Mode
        helper.exit_game_mode(Tests.__dict__["exit_game_mode_{}".format(test.test_index)])

    # 3) Verify that logged attributes show both levels are the same before and after the change in default material library
    #   and show that there was a change before and after the change in default material library
    sphere_final_velocities_0, hit_trigger_list_0 = get_data_from_previous_tests()

    sphere_final_velocities = sphere_final_velocities_0 + [test.final_velocity for test in test_list]
    hit_trigger_list = hit_trigger_list_0 + [test.trigger_triggered for test in test_list]
    Report.result(Tests.levels_start_equal, compare_level_baseline(sphere_final_velocities, hit_trigger_list))
    Report.result(
        Tests.material_library_switch,
        default_material_library_changed_as_expected(sphere_final_velocities, hit_trigger_list),
    )
    Report.result(
        Tests.levels_stay_equal, levels_coinsistent_after_modification(sphere_final_velocities, hit_trigger_list)
    )



if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(Material_DefaultLibraryUpdatedAcrossLevels_after)
