"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

# Test case ID : 4044455
# Test Case Title : Verify that any change in any of the values including the name of the material, 
#   once saved, is immediately reflected in the component and functionality


# fmt: off
class Tests:
    enter_game_mode_0           = ("Entered game mode 0",                     "Failed to enter game mode 0")
    exit_game_mode_0            = ("Exited game mode 0",                      "Couldn't exit game mode 0")
    enter_game_mode_1           = ("Entered game mode 1",                     "Failed to enter game mode 1")
    exit_game_mode_1            = ("Exited game mode 1",                      "Couldn't exit game mode 1")
    terrain_found_0             = ("terrain entity found 0",                  "terrain entity not found 0")
    block_found_0               = ("block entity found 0",                    "block entity not found 0")
    trigger_found_0             = ("trigger entity found 0",                  "trigger entity not found 0")
    terrain_found_1             = ("terrain entity found 1",                  "terrain entity not found 1")
    block_found_1               = ("block entity found 1",                    "block entity not found 1")
    trigger_found_1             = ("trigger entity found 1",                  "trigger entity not found 1")
    material_changes            = ("material changes were made",              "material changes couldn't be made")
    # Material Modifications
    static_friction             = ("Static friction was modified",            "Static friction wasn't modified")
    dynamic_friction            = ("Dynamic friction was modified",           "Dynamic friction wasn't modified")
    restitution                 = ("Restitution was modified",                "Restition wasn't modified")
    friction_combine            = ("Friction combine was modified",           "Friction combine wasn't modified")
    restitution_combine         = ("Restition combine was modified",          "Restitution combine wasn't modified")
    delete_material             = ("Material deleted successfully",           "Material wasn't deleted")

    # sphere_0 test 0
    sphere_0_found_0            = ("Test 0: sphere_0 found",                  "Test 0: sphere_0 not found")
    sphere_0_initial_position_0 = ("Test 0: sphere_0 is in valid position",   "Test 0: sphere_0 isn't in valid position")
    sphere_0_initial_velocity_0 = ("Test 0: sphere_0 initial velocity valid", "Test 0: sphere_0 initial velocity invalid")
    sphere_0_collision_0        = ("Test 0: sphere_0 collided with terrain",  "Test 0: sphere_0 collided with terrain")
    sphere_0_final_position_0   = ("Test 0: sphere_0 final position valid",   "Test 0: sphere_0 final position invalid")
    sphere_0_final_velocity_0   = ("Test 0: sphere_0 final velocity valid",   "Test 0: sphere_0 final velocity invalid")
    # sphere_0 test 1
    sphere_0_found_1            = ("Test 1: sphere_0 found",                  "Test 1: sphere_0 not found")
    sphere_0_initial_position_1 = ("Test 1: sphere_0 is in valid position",   "Test 1: sphere_0 isn't in valid position")
    sphere_0_initial_velocity_1 = ("Test 1: sphere_0 initial velocity valid", "Test 1: sphere_0 initial velocity invalid")
    sphere_0_collision_1        = ("Test 1: sphere_0 collided with terrain",  "Test 1: sphere_0 collided with terrain")
    sphere_0_final_position_1   = ("Test 1: sphere_0 final position valid",   "Test 1: sphere_0 final position invalid")
    sphere_0_final_velocity_1   = ("Test 1: sphere_0 final velocity valid",   "Test 1: sphere_0 final velocity invalid")
    # sphere_1 test 0
    sphere_1_found_0            = ("Test 0: sphere_1 found",                  "Test 0: sphere_1 not found")
    sphere_1_initial_position_0 = ("Test 0: sphere_1 is in valid position",   "Test 0: sphere_1 isn't in valid position")
    sphere_1_initial_velocity_0 = ("Test 0: sphere_1 initial velocity valid", "Test 0: sphere_1 initial velocity invalid")
    sphere_1_collision_0        = ("Test 0: sphere_1 collided with terrain",  "Test 0: sphere_1 collided with terrain")
    sphere_1_final_position_0   = ("Test 0: sphere_1 final position valid",   "Test 0: sphere_1 final position invalid")
    sphere_1_final_velocity_0   = ("Test 0: sphere_1 final velocity valid",   "Test 0: sphere_1 final velocity invalid")
    # sphere_1 test 1
    sphere_1_found_1            = ("Test 1: sphere_1 found",                  "Test 1: sphere_1 not found")
    sphere_1_initial_position_1 = ("Test 1: sphere_1 is in valid position",   "Test 1: sphere_1 isn't in valid position")
    sphere_1_initial_velocity_1 = ("Test 1: sphere_1 initial velocity valid", "Test 1: sphere_1 initial velocity invalid")
    sphere_1_collision_1        = ("Test 1: sphere_1 collided with terrain",  "Test 1: sphere_1 collided with terrain")
    sphere_1_final_position_1   = ("Test 1: sphere_1 final position valid",   "Test 1: sphere_1 final position invalid")
    sphere_1_final_velocity_1   = ("Test 1: sphere_1 final velocity valid",   "Test 1: sphere_1 final velocity invalid")
    # sphere_2 test 0
    sphere_2_found_0            = ("Test 0: sphere_2 found",                  "Test 0: sphere_2 not found")
    sphere_2_initial_position_0 = ("Test 0: sphere_2 is in valid position",   "Test 0: sphere_2 isn't in valid position")
    sphere_2_initial_velocity_0 = ("Test 0: sphere_2 initial velocity valid", "Test 0: sphere_2 initial velocity invalid")
    sphere_2_collision_0        = ("Test 0: sphere_2 collided with terrain",  "Test 0: sphere_2 collided with terrain")
    sphere_2_final_position_0   = ("Test 0: sphere_2 final position valid",   "Test 0: sphere_2 final position invalid")
    sphere_2_final_velocity_0   = ("Test 0: sphere_2 final velocity valid",   "Test 0: sphere_2 final velocity invalid")
    # sphere_2 test 1
    sphere_2_found_1            = ("Test 1: sphere_2 found",                  "Test 1: sphere_2 not found")
    sphere_2_initial_position_1 = ("Test 1: sphere_2 is in valid position",   "Test 1: sphere_2 isn't in valid position")
    sphere_2_initial_velocity_1 = ("Test 1: sphere_2 initial velocity valid", "Test 1: sphere_2 initial velocity invalid")
    sphere_2_collision_1        = ("Test 1: sphere_2 collided with terrain",  "Test 1: sphere_2 collided with terrain")
    sphere_2_final_position_1   = ("Test 1: sphere_2 final position valid",   "Test 1: sphere_2 final position invalid")
    sphere_2_final_velocity_1   = ("Test 1: sphere_2 final velocity valid",   "Test 1: sphere_2 final velocity invalid")

    # cube_0 test 0
    cube_0_found_0              = ("Test 0: cube_0 found",                    "Test 0: cube_0 not found")
    cube_0_initial_position_0   = ("Test 0: cube_0 is in correct position",   "Test 0: cube_0 isn't in correct position")
    cube_0_initial_velocity_0   = ("Test 0: cube_0 initial velocity valid",   "Test 0: cube_0 initial velocity invalid")
    cube_0_final_position_0     = ("Test 0: cube_0 has stopped moving",       "Test 0: cube_0 hasn't stopped moving")
    cube_0_final_velocity_0     = ("Test 0: cube_0 final velocity valid",     "Test 0: cube_0 final velocity invalid")
    # cube_0 test 1
    cube_0_found_1              = ("Test 1: cube_0 found",                    "Test 1: cube_0 not found")
    cube_0_initial_position_1   = ("Test 1: cube_0 is in correct position",   "Test 1: cube_0 isn't in correct position")
    cube_0_initial_velocity_1   = ("Test 1: cube_0 initial velocity valid",   "Test 1: cube_0 initial velocity invalid")
    cube_0_final_position_1     = ("Test 1: cube_0 has stopped moving",       "Test 1: cube_0 has not stopped moving")
    cube_0_final_velocity_1     = ("Test 1: cube_0 final velocity valid",     "Test 1: cube_0 final velocity invalid")
    # cube_1 test 0
    cube_1_found_0              = ("Test 0: cube_1 found",                    "Test 0: cube_1 not found")
    cube_1_initial_position_0   = ("Test 0: cube_1 is in correct position",   "Test 0: cube_1 isn't in correct position")
    cube_1_initial_velocity_0   = ("Test 0: cube_1 initial velocity valid",   "Test 0: cube_1 initial velocity invalid")
    cube_1_final_position_0     = ("Test 0: cube_1 has stopped moving",       "Test 0: cube_1 hasn't stopped moving")
    cube_1_final_velocity_0     = ("Test 0: cube_1 final velocity valid",     "Test 0: cube_1 final velocity invalid")
    # cube_1 test 1
    cube_1_found_1              = ("Test 1: cube_1 found",                    "Test 1: cube_1 not found")
    cube_1_initial_position_1   = ("Test 1: cube_1 is in correct position",   "Test 1: cube_1 isn't in correct position")
    cube_1_initial_velocity_1   = ("Test 1: cube_1 initial velocity valid",   "Test 1: cube_1 initial velocity invalid")
    cube_1_final_position_1     = ("Test 1: cube_1 has stopped moving",       "Test 1: cube_1 hasn't stopped moving")
    cube_1_final_velocity_1     = ("Test 1: cube_1 final velocity valid",     "Test 1: cube_1 final velocity invalid")
    # cube_2 test 0
    cube_2_found_0              = ("Test 0: cube_2 found",                    "Test 0: cube_2 not found")
    cube_2_initial_position_0   = ("Test 0: cube_2 is in correct position",   "Test 0: cube_2 isn't in correct position")
    cube_2_initial_velocity_0   = ("Test 0: cube_2 initial velocity valid",   "Test 0: cube_2 initial velocity invalid")
    cube_2_final_position_0     = ("Test 0: cube_2 has stopped moving",       "Test 0: cube_2 hasn't stopped moving")
    cube_2_final_velocity_0     = ("Test 0: cube_2 final velocity valid",     "Test 0: cube_2 final velocity invalid")
    # cube_2 test 1
    cube_2_found_1              = ("Test 1: cube_2 found",                    "Test 1: cube_2 not found")
    cube_2_initial_position_1   = ("Test 1: cube_2 is in correct position",   "Test 1: cube_2 isn't in correct position")
    cube_2_initial_velocity_1   = ("Test 1: cube_2 initial velocity valid",   "Test 1: cube_2 initial velocity invalid")
    cube_2_final_position_1     = ("Test 1: cube_2 has stopped moving",       "Test 1: cube_2 hasn't stopped moving")
    cube_2_final_velocity_1     = ("Test 1: cube_2 final velocity valid",     "Test 1: cube_2 final velocity invalid")

# fmt: on


def Material_LibraryChangesReflectInstantly():
    """
    Summary: Verify that any change in any of the values of the material, once saved, is immediately reflected
        in the component and functionality

    Level Description:
    Three sphere entities (sphere_0, sphere_1, sphere_2) - They start between the terrain and trigger with
        velocity of 10 m/s in the negative z direction; has physx collider with sphere shape, physx rigid body,
        sphere shape, has "to_change_restitution", "to_change_restitution_combine", and "to_delete" materials
        applied respectively.
    Three cube entities (cube_0, cube_1, cube_2) - On top of the negative y side of the block, gravity enabled, no
        initial velocity, 0.0 linear damping; has physx collider with box shape, physx rigid body, box shape, and
        has "to_change_static_friction", "to_change_dynamic_friction", and "to_change_friction_combine" materials
        applied respectively
    trigger - Stationary trigger above the three spheres, used to indicate if the material was modified correctly; has 
        physx collider with box shape (20.0, 5.0, 0.25) and trigger enabled and box shape (20.0, 5.0, 0.25)
    block - Stationary block that has all cubes sitting on it. Used as a controlled surface for friction testing; has
        physx collider with box shape (10.0, 10.0, 10.0) and box shape (10.0, 10.0, 10.0)
    terrain - terrain component holder lined up with terrain default height; has terrain component

    Material Library: Contains a different material for each entity with distinct collider shape. These materials are
        designed to provide the largest difference in result after change (sphere: velocity, cube: distance). All spheres 
        should not bounce off of the terrain initially but will be able to hit the trigger post change. The cubes will
        experience higher friction after the change and not travel as far along the ramp entity.

    Expected Behavior: Before editing the material library the spheres in both levels will not bounce off of the terrain
        and the cubes will go some distance along the ramp. After the material file is edited the spheres will bounce off
        of the terrain and hit the trigger and the cubes will travel a smaller distance than before

    Main Script Steps:
    1) Open Level
    2) Create test objects
    3) Run test 0
    4) Modify material library
    5) Run test 1
    6) Validate results
    7) Close Editor

    Test Loop Steps:
    1) Enter game mode
    2) Find and Validate entities
    3) Wait for spheres to collide with terrain
    4) Wait for spheres to enter the trigger
    5) Log sphere results
    6) Push cubes
    7) Wait for cubes to stop moving
    8) Log and validate cube results
    9) Exit game mode

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
    FLOAT_THRESHOLD = 0.001
    # Timeout in seconds
    TIMEOUT = 2.0
    CUBE_IMPULSE = math.Vector3(0.0, 5.0, 0.0)
    CUBE_Y_POSITION = 536.0
    CUBE_INITIAL_VELOCITY = math.Vector3(0.0, 0.0, 0.0)
    PROPAGATION_FRAMES = 500

    # Helper Functions
    class Entity:
        terrain_id = None
        def __init__(self, name, test_index):
            # Type (str, int, int, Entity) -> None
            self.id = general.find_game_entity(name)
            self.name = name
            self.test_index = test_index
            self.collision_happened = False
            self.hit_trigger = False
            # Check Entity ID
            found = Tests.__dict__["{}_found_{}".format(self.name, self.test_index)]
            Report.critical_result(found, self.id.IsValid())

        @property
        def position(self):
            return azlmbr.components.TransformBus(azlmbr.bus.Event, "GetWorldTranslation", self.id)

        @property
        def velocity(self):
            return azlmbr.physics.RigidBodyRequestBus(azlmbr.bus.Event, "GetLinearVelocity", self.id)

        @property
        def is_moving_up(self):
            # Type () -> bool
            return (
                abs(self.velocity.x) < FLOAT_THRESHOLD
                and abs(self.velocity.y) < FLOAT_THRESHOLD
                and self.velocity.z > 0.0
            )

        @property
        def is_not_moving(self):
            # Type () -> bool
            return (
                abs(self.velocity.x) < FLOAT_THRESHOLD
                and abs(self.velocity.y) < FLOAT_THRESHOLD
                and abs(self.velocity.z) < FLOAT_THRESHOLD
            )

        def on_collision_begin(self, args):
            # Type ([]) -> None
            if Entity.terrain_id.equal(args[0]):
                self.collision_happened = True
    
    class Sphere(Entity):
        def __init__(self, name, test_index):
            Entity.__init__(self, name, test_index)
            self.handler = azlmbr.physics.CollisionNotificationBusHandler()
            self.handler.connect(self.id)
            self.handler.add_callback("OnCollisionBegin", self.on_collision_begin)

    class Material_Test:
        def __init__(self, index):
            self.index = index
            self.sphere_list = None
            # List to hold how far the cube traveled
            self.cube_distances = []
            # List to hold wether the sphere hit the trigger and its velocities
            self.sphere_values = []

        def verify_sphere_initial_position(self, sphere, terrain, trigger):
            # Type (Entity, Entity, Entity) -> None
            # Validates sphere is where it should be
            position_valid = terrain.position.z < sphere.position.z < trigger.position.z
            initial_position = Tests.__dict__["{}_initial_position_{}".format(sphere.name, self.index)]
            Report.critical_result(initial_position, position_valid)

        def verify_sphere_initial_velocity(self, sphere):
            # Type (Entity) -> None
            # Validates that sphere in moving in the correct direction
            initial_velocity = Tests.__dict__["{}_initial_velocity_{}".format(sphere.name, self.index)]
            Report.critical_result(initial_velocity, not sphere.is_moving_up)

        def verify_sphere_collision(self, sphere):
            # Type (Entity) -> None
            # Reports sphere collision, ends test if it hasn't occurred
            collision = Tests.__dict__["{}_collision_{}".format(sphere.name, self.index)]
            Report.critical_result(collision, sphere.collision_happened)

        def verify_sphere_final_velocity(self, sphere):
            # Type (Entity) -> None
            # Validates that sphere is moving in the correct direction
            final_velocity = Tests.__dict__["{}_final_velocity_{}".format(sphere.name, self.index)]
            Report.result(final_velocity, sphere.is_moving_up or sphere.is_not_moving)

        def verify_sphere_final_position(self, sphere, terrain):
            # Type (Entity, Entity) -> None
            # Validats that sphere is not where it shouldn't be
            final_position = Tests.__dict__["{}_final_position_{}".format(sphere.name, self.index)]
            Report.result(final_position, sphere.position.z > terrain.position.z)

        def verify_cube_initial_position(self, cube, block):
            # Type (Entity, Entity) -> None
            # Cube initially starts at a standstill
            initial_position = Tests.__dict__["{}_initial_position_{}".format(cube.name, self.index)]
            Report.result(
                initial_position,
                cube.position.z > block.position.z and abs(cube.position.y - CUBE_Y_POSITION) < FLOAT_THRESHOLD,
            )

        def verify_cube_initial_velocity(self, cube):
            # Type (Entity) -> None
            # Ensures that the cube starts not moving
            initial_velocity = Tests.__dict__["{}_initial_velocity_{}".format(cube.name, self.index)]
            Report.result(initial_velocity, cube.velocity.IsClose(CUBE_INITIAL_VELOCITY, 0.01))

        def push_cubes(self, cube_list):
            # Type ([Entity]) -> None
            # Imparts a velocity into each cube in the y-direction
            for cube in cube_list:
                azlmbr.physics.RigidBodyRequestBus(azlmbr.bus.Event, "SetLinearVelocity", cube.id, CUBE_IMPULSE)

        def verify_cube_final_velocity(self, cube):
            # Type (Entity) -> None
            # Ensures that cube has stopped moving
            final_velocity = Tests.__dict__["{}_final_velocity_{}".format(cube.name, self.index)]
            Report.result(final_velocity, cube.velocity.IsClose(CUBE_INITIAL_VELOCITY, 0.01))

        def verify_cube_final_position(self, cube, block):
            # Type (Entity, Entity) -> None
            # Validates that cube is not somewhere it shouldn't be
            final_position = Tests.__dict__["{}_final_position_{}".format(cube.name, self.index)]
            Report.result(final_position, cube.position.z > block.position.z)

        def log_values(self, entity):
            # Type (Entity) -> None
            # Logs needed values for comparison
            if isinstance(entity, Sphere):
                self.sphere_values.append([entity.velocity, entity.hit_trigger])
            else:
                self.cube_distances.append(entity.position)

        def set_trigger(self, trigger):
            self.handler = azlmbr.physics.TriggerNotificationBusHandler()
            self.handler.connect(trigger.id)
            self.handler.add_callback("OnTriggerEnter", self.on_trigger_enter)

        def on_trigger_enter(self, args):
            for sphere in self.sphere_list:
                if sphere.id.equal(args[0]):
                    sphere.hit_trigger = True

    def modify_material_library():
        # Type () -> bool
        # Uses a Physmaterial_Editor option to modify the material library associated with this level.
        # Changes are made to maximize the in level affect.
        material_library = Physmaterial_Editor("c4044455_material_librarychangesinstantly.physmaterial")
        dynamic_friction_modified = material_library.modify_material("to_change_dynamic_friction", "DynamicFriction", 10.0)
        static_friction_modified = material_library.modify_material("to_change_static_friction", "StaticFriction", 10.0)
        friction_combine_modified = material_library.modify_material(
            "to_change_friction_combine", "FrictionCombine", "Maximum"
        )
        restitution_combine_modified = material_library.modify_material(
            "to_change_restitution_combine", "RestitutionCombine", "Maximum"
        )
        restitution_modified = material_library.modify_material("to_change_restitution", "Restitution", 1.0)
        material_deleted = material_library.delete_material("to_delete")
        material_library.save_changes()
        return (
            material_deleted
            and dynamic_friction_modified
            and static_friction_modified
            and friction_combine_modified
            and restitution_combine_modified
            and restitution_modified
        )

    def check_sphere(sphere_values_0, sphere_values_1, index):
        # Type ([[vector3, bool]], [[vector3, bool]]) -> bool
        hit_trigger = not sphere_values_0[index][1] and sphere_values_1[index][1]
        velocity_valid = sphere_values_0[index][0].z < sphere_values_1[index][0].z
        return hit_trigger and velocity_valid

    def check_static_friction(cube_distances_0, cube_distances_1):
        # Type ([float],[float]) -> bool
        return cube_distances_0[0].y > cube_distances_1[0].y

    def check_dynamic_friction(cube_distances_0, cube_distances_1):
        # Type ([float],[float]) -> bool
        return cube_distances_0[1].y > cube_distances_1[1].y

    def check_friction_combine(cube_distances_0, cube_distances_1):
        # Type ([float],[float]) -> bool
        return cube_distances_0[2].y > cube_distances_1[2].y

    def run_test(test):
        # Type (Material_Test) -> None
        # This loop runs the test steps and logs data to the given Material_Test object
        # 1) Enter game mode
        helper.enter_game_mode(Tests.__dict__["enter_game_mode_{}".format(test.index)])

        # 2) Find and Validate entities
        terrain = Entity("terrain", test.index)
        Entity.terrain_id = terrain.id
        block = Entity("block", test.index)
        trigger = Entity("trigger", test.index)
        sphere_0 = Sphere("sphere_0", test.index)
        sphere_1 = Sphere("sphere_1", test.index)
        sphere_2 = Sphere("sphere_2", test.index)
        sphere_list = [sphere_0, sphere_1, sphere_2]
        cube_0 = Entity("cube_0", test.index)
        cube_1 = Entity("cube_1", test.index)
        cube_2 = Entity("cube_2", test.index)
        cube_list = [cube_0, cube_1, cube_2]
        test.sphere_list = sphere_list
        test.set_trigger(trigger)

        for sphere in sphere_list:
            test.verify_sphere_initial_position(sphere, terrain, trigger)
            test.verify_sphere_initial_velocity(sphere)

        for cube in cube_list:
            test.verify_cube_initial_position(cube, block)
            test.verify_cube_initial_velocity(cube)

        # 3) Wait for spheres to collide with terrain
        helper.wait_for_condition(lambda: all([sphere.collision_happened for sphere in sphere_list]), TIMEOUT)

        # 4) Wait for spheres to enter the trigger
        helper.wait_for_condition(lambda: all([sphere.hit_trigger for sphere in sphere_list]), TIMEOUT)
        for sphere in sphere_list:
            test.log_values(sphere)

        # 5) Log sphere results
        for sphere in sphere_list:
            test.verify_sphere_collision(sphere)
            test.verify_sphere_final_position(sphere, terrain)
            test.verify_sphere_final_velocity(sphere)

        # 6) Push cubes
        test.push_cubes(cube_list)

        # 7) Wait for cubes to stop moving
        helper.wait_for_condition(lambda: all([cube.is_not_moving for cube in cube_list]), TIMEOUT)

        # 8) Log and validate cube results
        for cube in cube_list:
            test.verify_cube_final_position(cube, block)
            test.verify_cube_final_velocity(cube)
            test.log_values(cube)

        # 9) Exit game mode
        helper.exit_game_mode(Tests.__dict__["exit_game_mode_{}".format(test.index)])

    # Main Script
    helper.init_idle()
    # 1) Open Level
    helper.open_level("Physics", "C4044455_Material_LibraryChangesInstantly")

    # 2) Create test objects
    test_0 = Material_Test(0)
    test_1 = Material_Test(1)

    # 3) Run test 0
    run_test(test_0)

    # 4) Modify material library
    Report.result(Tests.material_changes, modify_material_library())

    # Wait for modifications to the material library to propagate.
    general.idle_wait_frames(PROPAGATION_FRAMES)

    # 5) Run test 1
    run_test(test_1)

    # 6) Validate results
    # Restitution Modification Successful
    Report.result(Tests.restitution, check_sphere(test_0.sphere_values, test_1.sphere_values, index=0))
    # Static Friction Modification Successful
    Report.result(Tests.static_friction, check_static_friction(test_0.cube_distances, test_1.cube_distances))
    # Dynamic Friction Modification Successful
    Report.result(Tests.dynamic_friction, check_dynamic_friction(test_0.cube_distances, test_1.cube_distances))
    # Friction Combine Modification Successful
    Report.result(Tests.friction_combine, check_friction_combine(test_0.cube_distances, test_1.cube_distances))
    # Restitution Combine Modification Successful
    Report.result(Tests.restitution_combine, check_sphere(test_0.sphere_values, test_1.sphere_values, index=1))
    # Material Delete Successful
    Report.result(Tests.delete_material, check_sphere(test_0.sphere_values, test_1.sphere_values, index=2))


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(Material_LibraryChangesReflectInstantly)
