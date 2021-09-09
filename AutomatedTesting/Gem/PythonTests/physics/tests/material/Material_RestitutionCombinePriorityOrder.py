"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

# Test case ID : C18981526
# Test Case Title : Verify when two objects with different materials collide, the restitution combine priority works



# fmt: off
class Tests():
    enter_game_mode     = ("Entered game mode",                              "Failed to enter game mode")
    find_box_average    = ("Box entity 'average' found",                     "Box entity 'average' not found")
    find_box_minimum    = ("Box entity 'minimum' found",                     "Box entity 'minimum' not found")
    find_box_multiply   = ("Box entity 'multiply' found",                    "Box entity 'multiply' not found")
    find_box_maximum    = ("Box entity 'maximum' found",                     "Box entity 'maximum' not found")
    find_ramp_average   = ("Ramp entity 'average' found",                    "Ramp entity 'average' not found")
    find_ramp_minimum   = ("Ramp entity 'minimum' found",                    "Ramp entity 'minimum' not found")
    find_ramp_multiply  = ("Ramp entity 'multiply' found",                   "Ramp entity 'multiply' not found")
    find_ramp_maximum   = ("Ramp entity 'maximum' found",                    "Ramp entity 'maximum' not found")

    # Test 0, first row of matrix
    boxes_fell_0        = ("Test 0): All boxes fell",                        "Test 0): All boxes did not fall")
    boxes_hit_ramp_0    = ("Test 0): All boxes hit the ramp",                "Test 0): All boxes did not hit the ramp")
    boxes_peaked_0      = ("Test 0): All boxes reached their max height",    "Test 0): All boxes did not reach their max height before timeout")

    # Test 1, second row of matrix
    boxes_fell_1        = ("Test 1): All boxes fell",                        "Test 1): All boxes did not fall")
    boxes_hit_ramp_1    = ("Test 1): All boxes hit the ramp",                "Test 1): All boxes did not hit the ramp")
    boxes_peaked_1      = ("Test 1): All boxes reached their max height",    "Test 1): All boxes did not reach their max height before timeout")

    # Test 2, third row of matrix
    boxes_fell_2        = ("Test 2): All boxes fell",                        "Test 2): All boxes did not fall")
    boxes_hit_ramp_2    = ("Test 2): All boxes hit the ramp",                "Test 2): All boxes did not hit the ramp")
    boxes_peaked_2      = ("Test 2): All boxes reached their max height",    "Test 2): All boxes did not reach their max height before timeout")

    # Test 3, fourth row of matrix
    boxes_fell_3        = ("Test 3): All boxes fell",                        "Test 3): All boxes did not fall")
    boxes_hit_ramp_3    = ("Test 3): All boxes hit the ramp",                "Test 3): All boxes did not hit the ramp")
    boxes_peaked_3      = ("Test 3): All boxes reached their max height",    "Test 3): All boxes did not reach their max height before timeout")

    basis_row_unique    = ("All distances in Test 0 were unique",            "All distances in Test 0 were not unique")
    basis_row_ordered   = ("All distances in Test 0 were correctly ordered", "All distances in Test 0 were not correctly ordered")
    height_matrix_valid = ("The resulting height matrix was valid",          "The resulting height matrix was invalid")

    exit_game_mode      = ("Exited game mode",                               "Couldn't exit game mode")
# fmt: on


def Material_RestitutionCombinePriorityOrder():
    """
    Summary:
    Runs an automated test to ensure that the restitution combine mode is assigned according to the correct priority.

    Level Description:
    Four boxes sit above one of 4 horizontal ramps.
    The ramps are identical, as are the boxes, save for their physX material:

    A new material library was created with 8 materials:
    minimum_box, multiply_box, average_box, maximum_box, minimum_ramp, multiply_ramp, average_ramp, maximum_ramp,
    Each 'box' material has its 'restitution combine' mode assigned as named; as well as the following properties:
        dynamic friction: 0.25
        static friction:  0.25
        restitution:      0.25
    The 'ramp' materials are assigned similarly, with the following values:
        dynamic friction: 0.5
        static friction:  0.5
        restitution:      0.5

    The values were specifically chosen to give a well-ordered and distributed result across all 4 combine modes
    (each progressive tier in priority gives a result 0.125 away from the last)

    Each box and ramp is assigned its corresponding restitution material
    Each box and ramp also has a PhysX box collider with default settings

    Expected Behavior:
    When two bodies with different materials are in contact, the physics system chooses which combine mode to use based
    on which combine mode has the highest priority.

    The priority order is as follows: Average < Minimum < Multiply < Maximum.

    For each ramp, this script drops the four boxes and measures their bounce height

    Upon collecting all data, the script evaluates the bounce height against an expected pattern.
    This pattern is derived from the priority order, to determine which combine mode should "win" over the others.
    Boxes with greater restitution combine coefficients should bounce higher.

    [Coefficient Combination Mode Results]
        average:  (0.25 +  0.5) / 2 -> 0.375
        minimum:   0.25 vs 0.5      -> 0.25
        multiply:  0.25 *  0.5      -> 0.125
        maximum:   0.25 vs 0.5      -> 0.5

    [Coefficient Combination Matrix]
                           Boxes
                avg   min   mul   max
           avg 0.375 0.25  0.125 0.5   # Test 0
    Ramps  min 0.25  0.25  0.125 0.5   # Test 1
           mul 0.125 0.125 0.125 0.5   # Test 2
           max 0.5   0.5   0.5   0.5   # Test 3

    Test Steps:
    1) Open level
    2) Enter game mode
    3) Validate entities

    For each ramp:
        4) Replace the ramp under the boxes
        5) Drop the boxes
        6) Wait for the box to hit the ground
        7) Measure the bounce height

    8) Validate matrix
    9) Exit game mode
    10) Close the editor

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

    import azlmbr
    import azlmbr.legacy.general as general
    import azlmbr.bus as bus
    import azlmbr.math as lymath

    NUMBER_OF_TESTS = 4
    DISTANCE_TOLERANCE = 0.005
    TIMEOUT = 5.0
    STANDBY_OFFSET = lymath.Vector3(0.0, 0.0, 4.0)
    SET_PHYSICS_WAIT = 10

    # region Entity Classes
    class Box:
        def __init__(self, name, valid_test):
            self.name = name
            self.id = general.find_game_entity(name)
            self.start_position = self.get_position()
            self.hit_ramp_position = None
            self.valid_test = valid_test
            self.peaked = False

        def get_position(self):
            return azlmbr.components.TransformBus(bus.Event, "GetWorldTranslation", self.id)

        def set_position(self, value):
            return azlmbr.components.TransformBus(bus.Event, "SetWorldTranslation", self.id, value)

        def get_velocity(self):
            return azlmbr.physics.RigidBodyRequestBus(bus.Event, "GetLinearVelocity", self.id)

        def set_velocity(self, value):
            return azlmbr.physics.RigidBodyRequestBus(bus.Event, "SetLinearVelocity", self.id, value)

        def set_gravity_enabled(self, value):
            azlmbr.physics.RigidBodyRequestBus(bus.Event, "SetGravityEnabled", self.id, value)

        def set_physics_enabled(self, value):
            if value:
                azlmbr.physics.RigidBodyRequestBus(bus.Event, "EnablePhysics", self.id)
            else:
                azlmbr.physics.RigidBodyRequestBus(bus.Event, "DisablePhysics", self.id)
                
        def force_awake(self):
            azlmbr.physics.RigidBodyRequestBus(bus.Event, "ForceAwake", self.id)

    class Ramp:
        def __init__(self, name, valid_test):
            self.name = name
            self.id = general.find_game_entity(name)
            self.start_position = self.get_position()
            self.valid_test = valid_test
            self.create_handler()
            self.collided_with_boxes = set()

        def on_collision_begin(self, args):
            other_id = args[0]
            for box in all_boxes:
                if box.id.Equal(other_id):
                    Report.info("Collided with {}".format(box.name))
                    self.collided_with_boxes.add(box)
                    box.hit_ramp_position = box.get_position()

        def create_handler(self):
            self.handler = azlmbr.physics.CollisionNotificationBusHandler()
            self.handler.connect(self.id)
            self.handler.add_callback("OnCollisionBegin", self.on_collision_begin)

        def get_position(self):
            return azlmbr.components.TransformBus(bus.Event, "GetWorldTranslation", self.id)

        def set_position(self, value):
            return azlmbr.components.TransformBus(bus.Event, "SetWorldTranslation", self.id, value)

        def all_boxes_hit(self):
            return len(self.collided_with_boxes) == 4

    class TestInfo:
        def __init__(self):
            self.fell_tests = []
            self.hit_ramp_tests = []
            self.peaked_tests = []
            for i in range(NUMBER_OF_TESTS):
                self.fell_tests.append(get_test("boxes_fell", i))
                self.hit_ramp_tests.append(get_test("boxes_hit_ramp", i))
                self.peaked_tests.append(get_test("boxes_peaked", i))
    # endregion

    # region Helper Functions
    def get_test(test_name, test_number):
        return Tests.__dict__["{}_{}".format(test_name, test_number)]

    def reset_boxes():
        for box in all_boxes:
            box.peaked = False
            box.set_physics_enabled(False)

        # We can't enable the boxes as kinematic and set their position on the same frame
        general.idle_wait_frames(SET_PHYSICS_WAIT)

        for box in all_boxes:
            box.set_position(box.start_position)

        general.idle_wait_frames(SET_PHYSICS_WAIT)

        for box in all_boxes:
            box.set_physics_enabled(True)
            box.force_awake()
    # endregion

    # region wait_for_condition() Functions
    def drop_boxes():
        for box in all_boxes:
            box.set_gravity_enabled(True)

    def all_boxes_falling():
        for box in all_boxes:
            if box.get_velocity().z >= 0.0:
                return False
        return True

    def all_boxes_peaked():
        peaked_boxes = 0
        for box in all_boxes:
            if box.peaked:
                peaked_boxes += 1
            else:
                current_position = box.get_position()
                current_height = current_position.z - box.hit_ramp_position.z
                current_linear_velocity = box.get_velocity()
                if current_linear_velocity.z > 0.0:
                    box.bounce_height = current_height
                else:
                    Report.info("Box {} reached {:.3f}M high".format(box.name, box.bounce_height))
                    box.set_gravity_enabled(False)
                    box.set_velocity(lymath.Vector3(0.0, 0.0, 0.0))
                    box.peaked = True
        return peaked_boxes == 4
    # endregion

    # region Matrix Validation
    def validate_matrix(matrix):
        # type: (list[list]) -> bool
        """
        Returns True if the matrix matches the pattern expected based on the friction combine priority.

        :param matrix: the height matrix

        :return: True if the matrix closely matches the expected pattern
        """
        # The first test represents a basis value for what is expected when each of the combine modes "wins" the other.
        # This is because every mode beats 'average' (the first ramp we test with) We can compare the rest of the matrix
        # against this basis row to validate the rest of the matrix as long as we also guarantee that the basis is valid
        #
        # Resulting matrix should follow the pattern:
        # A B C D <- Test 0
        # B B C D <- Test 1
        # C C C D <- Test 2
        # D D D D <- Test 3

        basis_row = matrix[0]
        Report.critical_result(Tests.basis_row_unique, list_is_unique(basis_row))

        average = basis_row[0]
        minimum = basis_row[1]
        multiply = basis_row[2]
        maximum = basis_row[3]
        # Based on the resulting coefficients, we can expect each bounce height to be ordered in a specific way
        Report.critical_result(Tests.basis_row_ordered, maximum > average > minimum > multiply)

        def report_failure(test_index, box_index, expected):
            box_name = all_boxes[box_index].name
            Report.info(
                "Matrix validation failure:\n"
                "Bounce height for box '{}' on test {} was not close to the expected basis value\n"
                "Bounce height: {:.3f}\n"
                "Expected: {:.3f}".format(box_name, test_index, matrix[test_index][box_index], expected)
            )

        valid = True
        for row_index, row in enumerate(matrix):
            for column_index, value in enumerate(row):
                max_index = max(row_index, column_index)

                if not float_is_close(value, basis_row[max_index], DISTANCE_TOLERANCE):
                    report_failure(row_index, column_index, basis_row[max_index])
                    valid = False
        return valid

    def log_matrix(matrix):
        matrix_display_string = "\nResulting Height Matrix:\n"
        for row in matrix:
            for value in row:
                matrix_display_string += "{:.3f},".format(value)
            matrix_display_string += "\n"
        Report.info(matrix_display_string)

    def list_is_unique(target_list):
        return len(set(target_list)) == len(target_list)

    def float_is_close(value, target, tolerance):
        return abs(value - target) <= tolerance
    # endregion

    helper.init_idle()
    # 1) Open level
    helper.open_level("Physics", "Material_RestitutionCombinePriorityOrder")

    # 2) Enter game mode
    helper.enter_game_mode(Tests.enter_game_mode)

    # Set up our boxes
    box_average = Box("Average", Tests.find_box_average)
    box_minimum = Box("Minimum", Tests.find_box_minimum)
    box_multiply = Box("Multiply", Tests.find_box_multiply)
    box_maximum = Box("Maximum", Tests.find_box_maximum)
    all_boxes = (box_average, box_minimum, box_multiply, box_maximum)

    # Set up our ramps
    ramp_average = Ramp("RampAverage", Tests.find_ramp_average)
    ramp_minimum = Ramp("RampMinimum", Tests.find_ramp_minimum)
    ramp_multiply = Ramp("RampMultiply", Tests.find_ramp_multiply)
    ramp_maximum = Ramp("RampMaximum", Tests.find_ramp_maximum)
    all_ramps = (ramp_average, ramp_minimum, ramp_multiply, ramp_maximum)

    # Init our tests
    test_info = TestInfo()

    # 3) Validate entities
    for box in all_boxes:
        Report.critical_result(box.valid_test, box.id.IsValid())

    for ramp in all_ramps:
        Report.critical_result(ramp.valid_test, ramp.id.IsValid())

    # Setup ramp active position. The 'average' ramp is the first ramp, so we init to that.
    active_position = ramp_average.get_position()

    # fmt: off
    height_matrix = [[0.0, 0.0, 0.0, 0.0],  # Test 0 - Ramp 'Average'
                     [0.0, 0.0, 0.0, 0.0],  # Test 1 - Ramp 'Minimum'
                     [0.0, 0.0, 0.0, 0.0],  # Test 2 - Ramp 'Multiply'
                     [0.0, 0.0, 0.0, 0.0]]  # Test 3 - Ramp 'Maximum'
    # fmt: on

    for row_index in range(len(height_matrix)):
        Report.info("********Starting Test {}********".format(row_index))
        reset_boxes()

        # 4) Replace the ramp under the boxes
        ramp = all_ramps[row_index]
        ramp.set_position(active_position)

        # 5) Drop the boxes
        drop_boxes()

        fell_test = test_info.fell_tests[row_index]
        Report.critical_result(fell_test, helper.wait_for_condition(all_boxes_falling, TIMEOUT))

        # 6) Wait for the box to hit the ground
        hit_ramp_test = test_info.hit_ramp_tests[row_index]
        Report.critical_result(hit_ramp_test, helper.wait_for_condition(ramp.all_boxes_hit, TIMEOUT))

        # 7) Measure the bounce height
        peaked_test = test_info.peaked_tests[row_index]
        Report.critical_result(peaked_test, helper.wait_for_condition(all_boxes_peaked, TIMEOUT))

        for column_index in range(len(height_matrix[row_index])):
            # Register the height the boxes bounced
            box = all_boxes[column_index]
            height_matrix[row_index][column_index] = box.bounce_height

        ramp.set_position(ramp.start_position.Subtract(STANDBY_OFFSET))

    # 8) Validate matrix
    log_matrix(height_matrix)
    Report.result(Tests.height_matrix_valid, validate_matrix(height_matrix))

    # 9) Exit game mode
    helper.exit_game_mode(Tests.exit_game_mode)



if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(Material_RestitutionCombinePriorityOrder)
