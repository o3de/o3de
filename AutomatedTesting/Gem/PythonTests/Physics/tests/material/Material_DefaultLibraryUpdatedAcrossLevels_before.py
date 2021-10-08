"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

# Test case ID : C15096732
# Test Case Title : Verify Default material library works across different levels


# fmt: off
class Tests:
    # Game Mode 0
    enter_game_mode_0         = ("Entered game mode 0",                   "Failed to enter game mode 0")
    sphere_found_0            = ("Test 0: Sphere was found",              "Test 0: Sphere was not found")
    terrain_found_0           = ("Test 0: Terrrain Entity found",         "Test 0: Terrain Entity was not found")
    trigger_found_0           = ("Test 0: trigger found",                 "Test 0: trigger not found")
    sphere_initial_position_0 = ("Test 0: Sphere initial position valid", "Test 0: Sphere initial position not valid")
    sphere_initial_velocity_0 = ("Test 0: Sphere initial velocity valid", "Test 0: Sphere initial velocity not valid")
    sphere_collision_0        = ("Test 0: Sphere collided with Terrain",  "Test 0: Sphere did not collide")
    exit_game_mode_0          = ("Exited game mode 0",                    "Couldn't exit game mode 0")
    # Game Mode 1
    enter_game_mode_1         = ("Entered game mode 1",                   "Failed to enter game mode 1")
    sphere_found_1            = ("Test 1: Sphere was found",              "Test 1: Sphere was not found")
    terrain_found_1           = ("Test 1: Terrrain Entity found",         "Test 1: Terrain Entity was not found")
    trigger_found_1           = ("Test 1: trigger found",                 "Test 1: trigger not found")
    sphere_initial_position_1 = ("Test 1: Sphere initial position valid", "Test 1: Sphere initial position not valid")
    sphere_initial_velocity_1 = ("Test 1: Sphere initial velocity valid", "Test 1: Sphere initial velocity not valid")
    sphere_collision_1        = ("Test 1: Sphere collided with Terrain",  "Test 1: Sphere did not collide")
    exit_game_mode_1          = ("Exited game mode 1",                    "Couldn't exit game mode 1")

# fmt: on


def Material_DefaultLibraryUpdatedAcrossLevels_before():
    """
    Summary: Verify Default material library works across different levels, this is the first stage to the test.
        After the tests are run this script will save data into a text file and the editor closed.
        C15096732_Material_DefaultLibraryUpdatedAcrossLevels_a.physmaterial is the default material file for
        these two tests.

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

    Expected Behavior: For the two test run by this script the ball will not bounce from the terrain and will
        not hit the trigger

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
    3) Log results of two steps to a local tmp file
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

    # Constants
    TIMEOUT = 2.0
    INITIAL_VELOCITY = math.Vector3(0.0, 0.0, -10.0)
    INITIAL_VELOCITY_THRESHOLD = 0.1

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
            if args[0].Equal(self.entity_list[0].id):
                self.trigger_triggered = True

        def check_sphere_initial_position(self, position_valid):
            initial_position = Tests.__dict__["sphere_initial_position_{}".format(self.test_index)]
            Report.critical_result(initial_position, position_valid)

        def check_sphere_initial_velocity(self):
            initial_velocity_string = Tests.__dict__["sphere_initial_velocity_{}".format(self.test_index)]
            Report.critical_result(
                initial_velocity_string,
                self.entity_list[0].velocity.IsClose(INITIAL_VELOCITY, INITIAL_VELOCITY_THRESHOLD),
            )

        def check_sphere_collision(self):
            collision = Tests.__dict__["sphere_collision_{}".format(self.test_index)]
            Report.result(collision, self.terrain_collision)

    def save_test_data(data):
        try:
            with open(
                os.path.join(
                    os.getcwd(), "AutomatedTesting", "Levels", "Physics", "Material_DefaultLibraryUpdatedAcrossLevels", "_last_run_before_change_data.txt"
                ),"w") as data_file:
                for data_point in data:
                    data_file.write(str(data_point))
                    data_file.write("\n")
        except Exception as e:
            Report.info(e)
            helper.fail_fast("Could not save data of first two tests.")

    helper.init_idle()
    # 1) Create Test Objects
    test_0 = Material_Test(test_index=0, level=0)
    test_1 = Material_Test(test_index=1, level=1)
    test_list = [test_0, test_1]

    # 2) Run Game Mode steps once for each test
    for test in test_list:
        # 1) Open the correct level is open
        helper.open_level(
            "Physics",
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

    # 3) Log results of two steps to a local tmp file
    sphere_final_velocities = [
        [test.final_velocity.x, test.final_velocity.y, test.final_velocity.z] for test in test_list
    ]
    hit_trigger_list = [test.trigger_triggered for test in test_list]

    save_test_data(sphere_final_velocities + hit_trigger_list)



if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(Material_DefaultLibraryUpdatedAcrossLevels_before)
