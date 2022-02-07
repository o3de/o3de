"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

# Test case ID : C18977601
# Test Case Title : Verify that when two objects with different materials collide, the friction combine priority works



# fmt: off
class Tests():
    enter_game_mode       = ("Entered game mode",                              "Failed to enter game mode")
    find_box_average      = ("Box entity 'average' found",                     "Box entity 'average' not found")
    find_box_minimum      = ("Box entity 'minimum' found",                     "Box entity 'minimum' not found")
    find_box_multiply     = ("Box entity 'multiply' found",                    "Box entity 'multiply' not found")
    find_box_maximum      = ("Box entity 'maximum' found",                     "Box entity 'maximum' not found")
    find_ramp_average     = ("Ramp entity 'average' found",                    "Ramp entity 'average' not found")
    find_ramp_minimum     = ("Ramp entity 'minimum' found",                    "Ramp entity 'minimum' not found")
    find_ramp_multiply    = ("Ramp entity 'multiply' found",                   "Ramp entity 'multiply' not found")
    find_ramp_maximum     = ("Ramp entity 'maximum' found",                    "Ramp entity 'maximum' not found")

    # Test 0, first row of matrix
    boxes_at_rest_start_0 = ("Test 0): All boxes began test motionless",       "Test 0): All boxes did not begin test motionless")
    boxes_were_pushed_0   = ("Test 0): All boxes moved",                       "Test 0): All boxes did not move before timeout")
    boxes_at_rest_end_0   = ("Test 0): All boxes came to rest",                "Test 0): All boxes did not come to rest before timeout")

    # Test 1, second row of matrix
    boxes_at_rest_start_1 = ("Test 1): All boxes began test motionless",       "Test 1): All boxes did not begin test motionless")
    boxes_were_pushed_1   = ("Test 1): All boxes moved",                       "Test 1): All boxes did not move before timeout")
    boxes_at_rest_end_1   = ("Test 1): All boxes came to rest",                "Test 1): All boxes did not come to rest before timeout")

    # Test 2, third row of matrix
    boxes_at_rest_start_2 = ("Test 2): All boxes began test motionless",       "Test 2): All boxes did not begin test motionless")
    boxes_were_pushed_2   = ("Test 2): All boxes moved",                       "Test 2): All boxes did not move before timeout")
    boxes_at_rest_end_2   = ("Test 2): All boxes came to rest",                "Test 2): All boxes did not come to rest before timeout")

    # Test 3, fourth row of matrix
    boxes_at_rest_start_3 = ("Test 3): All boxes began test motionless",       "Test 3): All boxes did not begin test motionless")
    boxes_were_pushed_3   = ("Test 3): All boxes moved",                       "Test 3): All boxes did not move before timeout")
    boxes_at_rest_end_3   = ("Test 3): All boxes came to rest",                "Test 3): All boxes did not come to rest before timeout")

    basis_row_unique      = ("All distances in Test 0 were unique",            "All distances in Test 0 were not unique")
    basis_row_ordered     = ("All distances in Test 0 were correctly ordered", "All distances in Test 0 were correctly ordered")
    distance_matrix_valid = ("The resulting distance matrix was valid",        "The resulting distance matrix was invalid")

    exit_game_mode        = ("Exited game mode",                               "Couldn't exit game mode")
# fmt: on


def Material_FrictionCombinePriorityOrder():
    """
    Summary:
    Runs an automated test to ensure that the friction combine mode is assigned according to the correct priority.

    Level Description:
    Four boxes sit on one of 4 horizontal ramps.
    The ramps are identical, as are the boxes, save for their physX material:

    A new material library was created with 8 materials:
    minimum_box, multiply_box, average_box, maximum_box, minimum_ramp, multiply_ramp, average_ramp, maximum_ramp,
    Each 'box' material has its 'friction combine' mode assigned as named; as well as the following properties:
        dynamic friction: 0.25
        static friction:  0.25
        restitution:      0.25
    The 'ramp' materials are assigned similarly, with the following values:
        dynamic friction: 0.5
        static friction:  0.5
        restitution:      0.5

    The values were specifically chosen to give a well-ordered and distributed result across all 4 combine modes
    (each progressive tier in priority gives a result 0.125 away from the last)

    Each box and ramp is assigned its corresponding friction material
    Each box and ramp also has a PhysX box collider with default settings
    The COM of the boxes is placed on the plane (0, 0, -0.5), so as to remove any torque moments and resulting rotations

    Expected Behavior:
    When two bodies with different materials are in contact, the physics system chooses which combine mode to use based
    on which combine mode has the highest priority.

    The priority order is as follows: Average < Minimum < Multiply < Maximum.

    For each ramp, this script applies a force impulse in the world X direction to all four boxes.

    Upon collecting all data, the script evaluates the traveled distances against an expected pattern.
    This pattern is derived from the priority order, to determine which combine mode should "win" over the others.
    Boxes with greater friction combine coefficients should travel a shorter distance.

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
        5) Ensure all boxes are stationary
        6) Push the boxes and wait for them to come to rest

    7) Validate matrix
    8) Exit game mode
    9) Close the editor

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
    FORCE_IMPULSE = lymath.Vector3(5.0, 0.0, 0.0)
    STANDBY_OFFSET = lymath.Vector3(0.0, 0.0, 4.0)
    DISTANCE_TOLERANCE = 0.002
    TIMEOUT = 5

    # region Entity Classes
    class Box:
        def __init__(self, name, valid_test):
            self.name = name
            self.id = general.find_game_entity(name)
            self.start_position = self.get_position()
            self.valid_test = valid_test

        def is_stationary(self):
            velocity = azlmbr.physics.RigidBodyRequestBus(bus.Event, "GetLinearVelocity", self.id)
            return velocity.IsZero()

        def get_position(self):
            return azlmbr.components.TransformBus(bus.Event, "GetWorldTranslation", self.id)

    class Ramp:
        def __init__(self, name, valid_test):
            self.name = name
            self.id = general.find_game_entity(name)
            self.start_position = self.get_position()
            self.valid_test = valid_test

        def get_position(self):
            return azlmbr.components.TransformBus(bus.Event, "GetWorldTranslation", self.id)

        def set_position(self, value):
            return azlmbr.components.TransformBus(bus.Event, "SetWorldTranslation", self.id, value)

    def get_test(test_name, test_number):
        return Tests.__dict__["{}_{}".format(test_name, test_number)]

    class TestInfo:
        def __init__(self):
            self.at_rest_start_tests = []
            self.moved_tests = []
            self.at_rest_end_tests = []
            for i in range(NUMBER_OF_TESTS):
                self.at_rest_start_tests.append(get_test("boxes_at_rest_start", i))
                self.moved_tests.append(get_test("boxes_were_pushed", i))
                self.at_rest_end_tests.append(get_test("boxes_at_rest_end", i))

    # endregion

    # region wait_for_condition() Functions
    def push_boxes():
        for box in all_boxes:
            azlmbr.physics.RigidBodyRequestBus(bus.Event, "ApplyLinearImpulse", box.id, FORCE_IMPULSE)

    def all_boxes_stationary():
        for box in all_boxes:
            if not box.is_stationary():
                return False
        return True

    def all_boxes_moving():
        for box in all_boxes:
            if box.is_stationary():
                return False
        return True

    # endregion

    # region Matrix Validation
    def list_is_unique(target_list):
        return len(set(target_list)) == len(target_list)

    def float_is_close(value, target, tolerance):
        return abs(value - target) <= tolerance

    def validate_matrix(matrix):
        # type: (list[list]) -> bool
        """
        Returns True if the matrix matches the pattern expected based on the friction combine priority.

        :param matrix: the distance matrix

        :return: True if the matrix closely matches the expected pattern
        """
        # The first test represents a basis value for what is expected when each of the combine modes "wins" the other.
        # This is because every mode beats 'average' (the first ramp we test on). We can compare the rest of the matrix
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
        # Based on the resulting coefficients, we can expect each slide distance to be ordered a specific way
        Report.critical_result(Tests.basis_row_ordered, maximum < average < minimum < multiply)

        def report_failure(test_index, box_index, expected):
            box_name = all_boxes[box_index].name
            Report.info(
                "Matrix validation failure:\n"
                "Distance for box '{}' on test {} was not close to the expected basis value\n"
                "Actual:   {:.3f}\n"
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
        matrix_display_string = "\nResulting Distance Matrix:\n"
        for row in matrix:
            for value in row:
                matrix_display_string += "{:.3f},".format(value)
            matrix_display_string += "\n"
        Report.info(matrix_display_string)

    # endregion

    helper.init_idle()
    # 1) Open level
    helper.open_level("Physics", "Material_FrictionCombinePriorityOrder")

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

    # Setup ramp active and standby positions
    active_position = ramp_average.get_position()
    stand_by_position = active_position.Subtract(STANDBY_OFFSET)

    # fmt: off
    distance_matrix = [[0.0, 0.0, 0.0, 0.0],  # Test 0 - Ramp 'Average'
                       [0.0, 0.0, 0.0, 0.0],  # Test 1 - Ramp 'Minimum'
                       [0.0, 0.0, 0.0, 0.0],  # Test 2 - Ramp 'Multiply'
                       [0.0, 0.0, 0.0, 0.0]]  # Test 3 - Ramp 'Maximum'
    # fmt: on

    for row_index in range(NUMBER_OF_TESTS):
        Report.info("********Starting Test {}********".format(row_index))

        # 4) Replace the ramp under the boxes
        ramp = all_ramps[row_index]
        ramp.set_position(active_position)

        # 5) Ensure all boxes are stationary
        Report.result(
            test_info.at_rest_start_tests[row_index], helper.wait_for_condition(all_boxes_stationary, TIMEOUT)
        )

        # 6) Push the boxes and wait for them to come to rest
        push_boxes()

        moved_test = test_info.moved_tests[row_index]
        at_rest_end_test = test_info.at_rest_end_tests[row_index]
        Report.result(moved_test, helper.wait_for_condition(all_boxes_moving, TIMEOUT))
        Report.result(at_rest_end_test, helper.wait_for_condition(all_boxes_stationary, TIMEOUT))

        for column_index in range(NUMBER_OF_TESTS):
            # Register the distance the boxes travelled
            box = all_boxes[column_index]
            end_position = box.get_position()
            distance = end_position.GetDistance(box.start_position)

            distance_matrix[row_index][column_index] = distance
            Report.info("Box {} travelled {:.3f} meters".format(box.name, distance))
            box.start_position = end_position

        ramp.set_position(stand_by_position)

    # 7) Validate matrix
    log_matrix(distance_matrix)
    Report.result(Tests.distance_matrix_valid, validate_matrix(distance_matrix))

    # 8) Exit game mode
    helper.exit_game_mode(Tests.exit_game_mode)



if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(Material_FrictionCombinePriorityOrder)
