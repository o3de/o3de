"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

# Test Case ID    : C4925579
# Test Case Title : Check that any change (Add/Delete/Modify) made to the material surface in the material library reflects immediately in the PhysX Terrain layers



# fmt: off
class Tests:
    enter_game_mode_0                = ("Test 0) Entered game mode",                                            "Test 0) Failed to enter game mode")
    find_terrain_0                   = ("Test 0) The Terrain was found",                                        "Test 0) The Terrain was not found")
    find_on_default_box_0            = ("Test 0) Box on default was found",                                     "Test 0) Box on default was not found")
    find_on_modified_box_0           = ("Test 0) Box on modified was found",                                    "Test 0) Box on modified was not found")
    boxes_moved_0                    = ("Test 0) All boxes moved",                                              "Test 0) Boxes failed to move")
    boxes_at_rest_0                  = ("Test 0) All boxes came to rest",                                       "Test 0) Boxes failed to come to rest")
    exit_game_mode_0                 = ("Test 0) Exited game mode",                                             "Test 0) Failed to exit game mode")
    on_default_less_than_on_modified = ("Test 0) Box on modified traveled farther than default",                "Test 0) Box on modified did not travel farther than default")

    enter_game_mode_1                = ("Test 1) Entered game mode",                                            "Test 1) Failed to enter game mode")
    find_terrain_1                   = ("Test 1) The Terrain was found",                                        "Test 1) The Terrain was not found")
    find_on_default_box_1            = ("Test 1) Box on default was found",                                     "Test 1) Box on default was not found")
    find_on_modified_box_1           = ("Test 1) Box on modified was found",                                    "Test 1) Box on modified was not found")
    boxes_moved_1                    = ("Test 1) All boxes moved",                                              "Test 1) Boxes failed to move")
    boxes_at_rest_1                  = ("Test 1) All boxes came to rest",                                       "Test 1) Boxes failed to come to rest")
    exit_game_mode_1                 = ("Test 1) Exited game mode",                                             "Test 1) Failed to exit game mode")
    on_modified_less_than_previous   = ("Test 1) Box on modified traveled less than previous",                  "Test 1) Box on modified traveled further than previous")

    enter_game_mode_2                = ("Test 2) Entered game mode",                                            "Test 2) Failed to enter game mode")
    find_terrain_2                   = ("Test 2) The Terrain was found",                                        "Test 2) The Terrain was not found")
    find_on_default_box_2            = ("Test 2) Box on default was found",                                     "Test 2) Box on default was not found")
    find_on_modified_box_2           = ("Test 2) Box on modified was found",                                    "Test 2) Box on modified was not found")
    boxes_moved_2                    = ("Test 2) All boxes moved",                                              "Test 2) Boxes failed to move")
    boxes_at_rest_2                  = ("Test 2) All boxes came to rest",                                       "Test 2) Boxes failed to come to rest")
    exit_game_mode_2                 = ("Test 2) Exited game mode",                                             "Test 2) Failed to exit game mode")
    on_default_equals_on_modified    = ("Test 2) The boxes on modified and default traveled the same distance", "Test 2) The boxes on modified and default did not travel the same distance")
# fmt: on


def Material_LibraryCrudOperationsReflectOnTerrain():
    """
    Summary:
    Runs an automated test to verify that any change (Add/Delete/Modify) made to the material surface in the material
    library reflects immediately in the PhysX Terrain layer component

    Level Description:
    Two boxes ("on_default" and "on_modified") sit on a terrain.

    The box "on_default" is placed on the terrain where the painted layer is the default physx material.
    A new material library was created with 1 material, called "Modified", this is painted on the terrain beneath "on_modified"
        dynamic friction: 0.25
        static friction:  0.5
        restitution:      0.5

    Expected behavior:
    For every iteration this test applies a force impulse in the X direction. The boxes save their traveled distances
    each iteration, to verify different behavior between each setup.

    First the test verifies the two entities sit upon differing materials, without changing anything. With a lower
    dynamic friction coefficient, the box 'on_modified' should travel a longer distance than 'on_default'

    Next, the test modifies the dynamic friction value for 'modified' (from 0.25 to 0.75). 'on_modified' should travel a
    shorter distance than it did in the previous test.

    Finally, we delete the 'modified' material entirely. The 'on_modified' box should then behave as the 'on_default'
    box, and travel the same distance.

    Test Steps:
    1) Open level
    2) Collect basis values without modifying anything
        2.1) Enter game mode
        2.2) Find entities
        2.3) Push the boxes and wait for them to come to rest
        2.4) Exit game mode
    3) Modify the dynamic friction value of 'modified'
        3.1 - 3.4) <same as above>
    4) Delete 'on_modified's material
        4.1 - 4.4) <same as above>
    5) Close editor

    Note:
    - This test file must be called from the Open 3D Engine Editor command terminal
    - Any passed and failed tests are written to the Editor.log file.
            Parsing the file or running a log_monitor are required to observe the test results.

    :return: None
    """
    import os
    import sys
    import azlmbr.legacy.general as general
    import azlmbr.math as lymath

    from Physmaterial_Editor import Physmaterial_Editor
    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper as helper
    from AddModifyDelete_Utils import Box

    FORCE_IMPULSE = lymath.Vector3(5.0, 0.0, 0.0)
    TIMEOUT = 3.0
    DISTANCE_TOLERANCE = 0.001

    def get_test(test_name):
        return Tests.__dict__[test_name]

    def run_test(test_number):

        # x.1) Enter game mode
        helper.enter_game_mode(get_test("enter_game_mode_{}".format(test_number)))

        # x.2) Find entities
        Report.result(get_test("find_terrain_{}".format(test_number)), general.find_game_entity("terrain").IsValid())
        Report.result(get_test("find_on_default_box_{}".format(test_number)), default_box.find())
        Report.result(get_test("find_on_modified_box_{}".format(test_number)), modified_box.find())

        # x.3) Push the boxes and wait for them to come to rest
        default_box.push(FORCE_IMPULSE)
        modified_box.push(FORCE_IMPULSE)

        def boxes_are_moving():
            return not default_box.is_stationary() and not modified_box.is_stationary()

        def boxes_are_stationary():
            return default_box.is_stationary() and modified_box.is_stationary()

        Report.result(
            get_test("boxes_moved_{}".format(test_number)), helper.wait_for_condition(boxes_are_moving, TIMEOUT),
        )
        Report.result(
            get_test("boxes_at_rest_{}".format(test_number)), helper.wait_for_condition(boxes_are_stationary, TIMEOUT),
        )

        default_box.distances.append(default_box.position.GetDistance(default_box.start_position))
        modified_box.distances.append(modified_box.position.GetDistance(modified_box.start_position))

        # x.4) Exit game mode
        helper.exit_game_mode(get_test("exit_game_mode_{}".format(test_number)))

    # 1) Open level and enter game mode
    helper.init_idle()
    helper.open_level("Physics", "Material_LibraryCrudOperationsReflectOnTerrain")

    # Setup persisting entities
    default_box = Box("on_default")
    modified_box = Box("on_modified")

    # 2) Collect basis values without modifying anything
    run_test(0)
    # While sitting on a terrain with friction of 0.25, 'on_modified' should travel farther than 'default'
    Report.result(Tests.on_default_less_than_on_modified, default_box.distances[0] < modified_box.distances[0])

    # 3) Modify the dynamic friction value of 'modified'
    material_editor = Physmaterial_Editor("c4925579_material_addmodifydeleteonterrain.physmaterial")
    material_editor.modify_material("Modified", "DynamicFriction", 0.75)
    material_editor.save_changes()
    run_test(1)
    # With greater friction, 'on_modified' should now travel a shorter distance than it did in the previous test.
    Report.result(Tests.on_modified_less_than_previous, modified_box.distances[0] > modified_box.distances[1])

    # 4) Delete 'modified's material
    material_editor.delete_material("Modified")
    material_editor.save_changes()
    run_test(2)
    Report.result(
        Tests.on_default_equals_on_modified,
        lymath.Math_IsClose(default_box.distances[2], modified_box.distances[2], DISTANCE_TOLERANCE),
    )


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(Material_LibraryCrudOperationsReflectOnTerrain)
