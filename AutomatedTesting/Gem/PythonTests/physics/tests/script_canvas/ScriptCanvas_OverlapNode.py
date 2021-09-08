"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

# Test case ID : 12712454
# Test Case Title : Verify overlap nodes in script canvas


# fmt: off
class Tests:
    enter_game_mode            = ("Entered game mode",               "Failed to enter game mode")
    exit_game_mode             = ("Exited game mode",                "Couldn't exit game mode")
    # comm entities are used for communication between script canvas and the python script.
    comm_entities_found        = ("All comm entities found",         "All comm entities not found")
    script_canvas_entity_found = ("Script_Canvas_Entity Found",      "Script_Canvas_Entity not found")
    test_array_found           = ("8x8 entity array found",          "8x8 entity array not found")
    all_tests_passed           = ("All tests have expected results", "Some tests had unexpected results")

    # Test 0
    test_0_finished            = ("Test 0 has ended",                "Test 0 has not ended correctly")
    test_0_results_logged      = ("Test 0 logged expected results",  "Test 0 logged unexpected results")
    test_0_entity_reset        = ("Test 0: entities moved back",     "Test 0: not all entities moved back")

    # Test 1
    test_1_finished            = ("Test 1 has ended",                "Test 1 has not ended correctly")
    test_1_results_logged      = ("Test 1 logged expected results",  "Test 1 logged unexpected results")
    test_1_entity_reset        = ("Test 1: entities moved back",     "Test 1: not all entities moved back")

    # Test 2
    test_2_finished            = ("Test 2 has ended",                "Test 2 has not ended correctly")
    test_2_results_logged      = ("Test 2 logged expected results",  "Test 2 logged unexpected results")
    test_2_entity_reset        = ("Test 2: entities moved back",     "Test 2: not all entities moved back")
# fmt: on


def ScriptCanvas_OverlapNode():
    """
    Summary: Verifies that the three script canvas overlap nodes (box, sphere, capsule) work as expected. Test
        script starts and verifies each case one by one.

    Level Description:
    Test Array - 8x8 array of sphere entities lined up edge to edge along an y-z plane, they are names with 
        'Sphere_row_column' convention; has PhysX rigid body, sphere shaped PhysX collider, sphere shape.
    Comm From Test- 3 communication entities that start inactive, used by the test script to initiate each of the three cases
        by activation of the relevent sphere; has sphere shape.
    Comm To Test- 3 communication entities that start inactive, used by the script canvas to initiate when each of the three
        cases are completed by activation of the relevent sphere; has sphere shape.
    Script Canvas Entity - Entity is centered in the test array and oriented with a 90 degree angle along the y
        axis as to allow the capsule overlap node to overlap more entities; has script canvas component

    Script Canvas - Has three execution cases, one for each overlap node type. Each case goes through a similar path
        - Waits for relevant Comm From Test entity to be activated by the test script
        - Creates an array of all entities in Test Array that overlap
        - Uses this array to draw a blue sphere at every sphere that overlaps
        - Uses the same array to move each overlapping entity 5 m along the +x axis
        - Activates the relevant Comm To Test entity
        The test script then logs the results and resets the Test Array before activating the next path.
        Values for the overlaps:
            Box: x = 4.5, y = 4.5, z = 4.5
            Sphere: r = 4
            Capsule: r = 0.5, h = 5
        Note: For this script canvas to run properly it requires the custom .physxconfiguration file.

    Tests Runs - Each loop runs a different overlap node
        - test_0: Box
        - test_1: Sphere
        - test_2: Capsule

    Expected Behavior: The script canvas will run it's three different overlap nodes. Each run the overlapped
        spheres in the Test Array will be drawn in over in blue and then offset in the x direction so that this test
        script can confirm that it worked correctly.

    Test Steps:
    1) Open Level
    2) Enter Game Mode
    3) Create Entity Objects
    4) Validate Entities
    5) Begin Testing
    6) Signal Script Canvas to begin test
    7) Wait until script canvas signals that it has completed the test
    8) Check which entities in array have been moved
    9) Validate and log results of test
    10) Place all entities back into correct positions in entity array
    11) Log results of all tests
    12) Exit Game Mode
    13) Close Editor

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
    # Timeout for a failed test in seconds
    TIMEOUT = 1.0
    ARRAY_X = 500.0
    ARRAY_COLUMN_0 = 530.0
    ARRAY_ROW_0 = 45.0

    # Helper Functions
    class Entity:
        def __init__(self, name, activated):
            self.id = general.find_game_entity(name)
            self.name = name
            self.activated = activated
            self.handler = None

        def activate_entity(self):
            Report.info("Activating Entity : " + self.name)
            azlmbr.entity.GameEntityContextRequestBus(azlmbr.bus.Broadcast, "ActivateGameEntity", self.id)
            self.activated = True

        def entity_activated(self, args):
            self.activated = True

        def set_handler(self):
            self.handler = azlmbr.entity.EntityBusHandler()
            self.handler.connect(self.id)
            self.handler.add_callback("OnEntityActivated", self.entity_activated)

    class Sphere_Entities:
        def __init__(self, row, column):
            self.name = "Sphere_{}_{}".format(row, column)
            self.id = general.find_game_entity(self.name)
            self.y = column + ARRAY_COLUMN_0
            self.z = ARRAY_ROW_0 - row
            self.array_position = math.Vector3(ARRAY_X, self.y, self.z)
            self.current_position = self.array_position

        def check_id(self):
            return self.id.isValid()

        def move_entity_back(self):
            azlmbr.components.TransformBus(azlmbr.bus.Event, "SetWorldTranslation", self.id, self.array_position)

        def in_position(self):
            current_position = azlmbr.components.TransformBus(azlmbr.bus.Event, "GetWorldTranslation", self.id)
            return abs(current_position.x - self.array_position.x) < FLOAT_THRESHOLD

    class Script_Canvas_Test:
        def __init__(self, index):
            self.index = index
            self.name = "test_{}".format(index)
            self.got_expected_result = False
            self.test_finish = None
            self.results_logged = None
            self.array_reset = None

        def report_test_start(self):
            Report.info("Test {} has begun".format(self.index))

        def report_test_finished(self, finished):
            self.test_finish = Tests.__dict__["test_{}_finished".format(self.index)]
            Report.result(self.test_finish, finished)

        def report_results_logged(self):
            self.results_logged = Tests.__dict__["test_{}_results_logged".format(self.index)]
            Report.result(self.results_logged, self.got_expected_result)

        def report_test_array_reset(self, entities_in_position):
            self.array_reset = Tests.__dict__["test_{}_entity_reset".format(self.index)]
            Report.result(self.array_reset, entities_in_position)

    # fmt: off
    class Result_Arrays:
        true_bool_array = [[True for column in range(8)] for row in range(8)]
        # Comparison Array for box overlap node
        test_0_bool_array = [
                [True, True,  True,  True,  True,  True,  True,  True],
                [True, False, False, False, False, False, False, True],
                [True, True, False, False, False, False, False, True],
                [True, False, False, False, False, False, False, True],
                [True, True, False, False, False, False, False, True],
                [True, True, False, False, False, False, False, True],
                [True, True, False, False, False, False, False, True],
                [True, True,  True,  True,  True,  True,  True,  True],
            ]

        # Comparison Array for sphere overlap node
        test_1_bool_array = [
                [True, True,  True,  True,  True,  True,  True,  True],
                [True, True,  False, False, False, False, True,  True],
                [True, False, False, False, False, False, False, True],
                [True, False, False, False, False, False, False, True],
                [True, False, False, False, False, False, False, True],
                [True, False, False, False, False, False, False, True],
                [True, True,  False, False, False, False, True,  True],
                [True, True,  True,  True,  True,  True,  True,  True],
            ]

        # Comparison Array for capsule overlap node
        test_2_bool_array = [
                [True, True, True, True,  True,  True, True, True],
                [True, True, True, False, False, True, True, True],
                [True, True, True, False, False, True, True, True],
                [True, True, True, False, False, True, True, True],
                [True, True, True, False, False, True, True, True],
                [True, True, True, False, False, True, True, True],
                [True, True, True, False, False, True, True, True],
                [True, True, True, True,  True,  True, True, True],
            ]
    # fmt on

    def create_test_Sphere_Entities():
        return [[Sphere_Entities(row, column) for column in range(8)] for row in range(8)]

    def sphere_array_isvalid(array):
        for row in range(len(array)):
            for column in range(len(array[0])):
                if not array[row][column].id.isValid():
                    return False
        return True

    def check_comm_entities(comm_list):
        for entity in comm_list:
            if not entity.id.isValid():
                return False
        return True

    def check_array_movement(array):
        return [[True if entity.in_position() else False for entity in row] for row in array]

    def reset_array(array):
        for row in array:
            for entity in row:
                entity.move_entity_back()

    def is_array_in_position(array):
        return arrays_are_the_same(Result_Arrays.true_bool_array, check_array_movement(array))

    def arrays_are_the_same(array_0, array_1):
        differences = [row for row in array_0 if row not in array_1] + [row for row in array_1 if row not in array_0]
        return differences == []

    # Main Script
    helper.init_idle()
    # 1) Open Level
    helper.open_level("Physics", "ScriptCanvas_OverlapNode")

    # 2) Enter Game Mode
    helper.enter_game_mode(Tests.enter_game_mode)

    # 3) Create Entity and Script_Canvas_Test Objects
    test_Sphere_Entities = create_test_Sphere_Entities()
    script_canvas_entity = Entity("Script_Canvas_Entity", True)
    comm_from_test_0 = Entity("Comm_From_Test_0", False)
    comm_from_test_1 = Entity("Comm_From_Test_1", False)
    comm_from_test_2 = Entity("Comm_From_Test_2", False)
    comm_to_test_0 = Entity("Comm_To_Test_0", False)
    comm_to_test_1 = Entity("Comm_To_Test_1", False)
    comm_to_test_2 = Entity("Comm_To_Test_2", False)
    comm_from_entity_list = (comm_from_test_0, comm_from_test_1, comm_from_test_2)
    comm_to_entity_list = (comm_to_test_0, comm_to_test_1, comm_to_test_2)

    test_0 = Script_Canvas_Test(0)
    test_1 = Script_Canvas_Test(1)
    test_2 = Script_Canvas_Test(2)
    test_list = (test_0, test_1, test_2)

    # 4) Validate Entities
    Report.critical_result(Tests.script_canvas_entity_found, script_canvas_entity.id.isValid())
    Report.critical_result(
        Tests.comm_entities_found, check_comm_entities(comm_from_entity_list + comm_to_entity_list)
        )
    Report.critical_result(Tests.test_array_found, sphere_array_isvalid(test_Sphere_Entities))

    # 5) Begin Testing
    for test in test_list:
        # 6) Signal Script Canvas to begin test
        comm_from_entity_list[test.index].activate_entity()
        test.report_test_start()

        # 7) Wait until script canvas signals that it has completed the test
        comm_to_entity_list[test.index].set_handler()
        test.report_test_finished(helper.wait_for_condition(lambda: comm_to_entity_list[test.index].activated, TIMEOUT))

        # 8) Check which entities in array have been moved
        result_bool_array = check_array_movement(test_Sphere_Entities)

        # 9) Validate and log results of test
        test.got_expected_result = arrays_are_the_same(
            result_bool_array, Result_Arrays.__dict__["test_{}_bool_array".format(test.index)]
        )
        test.report_results_logged()

        if test.index == 0:
            Report.info(result_bool_array)

        # 10) Place all entities back into correct positions in entity array
        reset_array(test_Sphere_Entities)
        test.report_test_array_reset(is_array_in_position(test_Sphere_Entities))

    # 11) Log results of all tests
    Report.result(
        Tests.all_tests_passed, test_0.got_expected_result and test_1.got_expected_result and test_2.got_expected_result
    )

    # 12) Exit Game Mode
    helper.exit_game_mode(Tests.exit_game_mode)


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(ScriptCanvas_OverlapNode)
