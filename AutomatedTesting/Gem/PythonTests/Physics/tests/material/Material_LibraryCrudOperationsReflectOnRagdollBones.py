"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

# Test Case ID    : C4925582
# Test Case Title : Check that any change (Add/Delete/Modify) made to the material surface in the material library reflects immediately in the ragdoll bones



# fmt: off
class Tests:
    enter_game_mode_0             = ("Test 0) Entered game mode",                                          "Test 0) Failed to enter game mode")
    find_terrain_0                = ("Test 0) The Terrain was found",                                      "Test 0) The Terrain was not found")
    find_default_ragdoll_0        = ("Test 0) Default ragdoll was found",                                  "Test 0) Default ragdoll was not found")
    find_modified_ragdoll_0       = ("Test 0) Modified ragdoll was found",                                 "Test 0) Modified ragdoll was not found")
    default_ragdoll_bounced_0     = ("Test 0) Default ragdoll bounced",                                    "Test 0) Default ragdoll did not bounce")
    modified_ragdoll_bounced_0    = ("Test 0) Modified ragdoll bounced",                                   "Test 0) Modified ragdoll did not bounce")
    exit_game_mode_0              = ("Test 0) Exited game mode",                                           "Test 0) Failed to exit game mode")
    modified_less_than_default    = ("Test 0) Modified ragdoll's bounce height was shorter than default",  "Test 0) Modified ragdoll's bounce height was greater than default")

    enter_game_mode_1             = ("Test 1) Entered game mode",                                          "Test 1) Failed to enter game mode")
    find_terrain_1                = ("Test 1) The Terrain was found",                                      "Test 1) The Terrain was not found")
    find_default_ragdoll_1        = ("Test 1) Default ragdoll was found",                                  "Test 1) Default ragdoll was not found")
    find_modified_ragdoll_1       = ("Test 1) Modified ragdoll was found",                                 "Test 1) Modified ragdoll was not found")
    default_ragdoll_bounced_1     = ("Test 1) Default ragdoll bounced",                                    "Test 1) Default ragdoll did not bounce")
    modified_ragdoll_bounced_1    = ("Test 1) Modified ragdoll bounced",                                   "Test 1) Modified ragdoll did not bounce")
    exit_game_mode_1              = ("Test 1) Exited game mode",                                           "Test 1) Failed to exit game mode")
    modified_greater_than_default = ("Test 1) Modified ragdoll's bounce height was higher than default's", "Test 1) Modified ragdoll's bounce height was not higher than default's")

    enter_game_mode_2             = ("Test 2) Entered game mode",                                          "Test 2) Failed to enter game mode")
    find_terrain_2                = ("Test 2) The Terrain was found",                                      "Test 2) The Terrain was not found")
    find_default_ragdoll_2        = ("Test 2) Default ragdoll was found",                                  "Test 2) Default ragdoll was not found")
    find_modified_ragdoll_2       = ("Test 2) Modified ragdoll was found",                                 "Test 2) Modified ragdoll was not found")
    default_ragdoll_bounced_2     = ("Test 2) Default ragdoll bounced",                                    "Test 2) Default ragdoll did not bounce")
    modified_ragdoll_bounced_2    = ("Test 2) Modified ragdoll bounced",                                   "Test 2) Modified ragdoll did not bounce")
    exit_game_mode_2              = ("Test 2) Exited game mode",                                           "Test 2) Failed to exit game mode")
    default_equals_modified       = ("Test 2) Modified and default ragdoll's bounce height were equal",    "Test 2) Modified and default ragdoll's bounce height were not equal")
# fmt: on


def Material_LibraryCrudOperationsReflectOnRagdollBones():
    """
    Summary:
    Runs an automated test to verify that any change (Add/Delete/Modify) made to the material surface in the material
    library reflects immediately in the ragdoll bones

    Level Description:
    Two ragdolls ("default_ragdoll" and "modified_ragdoll") sit above a terrain. The ragdolls are identical, save for
    their physX material.

    The ragdoll "default_ragdoll" is assigned the default physx material.
    A new material library was created with 1 material, called "Modified", this is assigned to "modified_ragdoll":
        dynamic friction: 0.5
        static friction:  0.5
        restitution:      0.25

    Expected behavior:
    For every iteration this test measures the bounce height of each entity. The ragdolls save their traveled distances
    each iteration, to verify different behavior between each setup.

    First the test verifies the two entities are assigned differing materials, without changing anything. With a lower
    restitution value, the 'modified' should bounce much lower than 'default'

    Next, the test modifies the restitution value for 'modified' (from 0.25 to 0.75). 'modified' should bounce height
    than it did in the previous test, and greater than default.

    Finally, we delete the 'modified' material entirely. 'modified_ragdoll' should then behave the same as
    'default_ragdoll' box, and bounce the same distance.

    Test Steps:
    1) Open level
    2) Collect basis values without modifying anything
        2.1) Enter game mode
        2.2) Find entities
        2.3) Wait for entities to bounce
        2.4) Exit game mode
    3) Modify the restitution value of 'modified'
        3.1 - 3.4) <same as above>
    4) Delete 'modified_ragdoll's material
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
    import azlmbr.bus as bus
    import azlmbr.components
    import azlmbr.physics
    import azlmbr.math as lymath

    from Physmaterial_Editor import Physmaterial_Editor
    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper as helper

    TIMEOUT = 3.0
    BOUNCE_TOLERANCE = 0.05

    class Ragdoll:
        def __init__(self, name):
            self.name = name
            self.bounces = []

        def find_and_reset(self):
            self.hit_terrain_position = None
            self.hit_terrain = False
            self.max_bounce = 0.0
            self.reached_max_bounce = False
            self.id = general.find_game_entity(self.name)
            return self.id.IsValid()

        @property
        def position(self):
            return azlmbr.components.TransformBus(bus.Event, "GetWorldTranslation", self.id)

    def get_test(test_name):
        return Tests.__dict__[test_name]

    def run_test(test_number):
        # x.1) Enter game mode
        helper.enter_game_mode(get_test("enter_game_mode_{}".format(test_number)))

        # x.2) Find entities
        terrain_id = general.find_game_entity("terrain")
        Report.result(get_test("find_terrain_{}".format(test_number)), terrain_id.IsValid())
        Report.result(get_test("find_default_ragdoll_{}".format(test_number)), default_ragdoll.find_and_reset())
        Report.result(get_test("find_modified_ragdoll_{}".format(test_number)), modified_ragdoll.find_and_reset())

        def on_collision_enter(args):
            entering = args[0]
            for ragdoll in ragdolls:
                if ragdoll.id.Equal(entering):
                    if not ragdoll.hit_terrain:
                        ragdoll.hit_terrain_position = ragdoll.position
                        ragdoll.hit_terrain = True

        handler = azlmbr.physics.CollisionNotificationBusHandler()
        handler.connect(terrain_id)
        handler.add_callback("OnCollisionBegin", on_collision_enter)

        def wait_for_bounce():
            for ragdoll in ragdolls:
                if ragdoll.hit_terrain:
                    current_bounce_height = ragdoll.position.z - ragdoll.hit_terrain_position.z
                    if current_bounce_height >= ragdoll.max_bounce:
                        ragdoll.max_bounce = current_bounce_height
                    elif ragdoll.max_bounce > 0.0:
                        ragdoll.reached_max_bounce = True
            return default_ragdoll.reached_max_bounce and modified_ragdoll.reached_max_bounce

        # x.3) Wait for entities to bounce
        helper.wait_for_condition(wait_for_bounce, TIMEOUT)
        Report.result(get_test("default_ragdoll_bounced_{}".format(test_number)), default_ragdoll.reached_max_bounce)
        Report.result(get_test("modified_ragdoll_bounced_{}".format(test_number)), modified_ragdoll.reached_max_bounce)

        for ragdoll in ragdolls:
            ragdoll.bounces.append(ragdoll.max_bounce)

        # x.4) Exit game mode
        helper.exit_game_mode(get_test("exit_game_mode_{}".format(test_number)))

    # 1) Open level and enter game mode
    helper.init_idle()
    helper.open_level("Physics", "Material_LibraryCrudOperationsReflectOnRagdollBones")

    # Setup persisting entities
    default_ragdoll = Ragdoll("default")
    modified_ragdoll = Ragdoll("modified")
    ragdolls = [default_ragdoll, modified_ragdoll]

    # 2) Collect basis values without modifying anything
    run_test(0)
    Report.result(Tests.modified_less_than_default, default_ragdoll.bounces[0] > modified_ragdoll.bounces[0])

    # 3) Modify the restitution value of 'modified'
    material_editor = Physmaterial_Editor("ragdollbones.physmaterial")
    material_editor.modify_material("Modified", "Restitution", 0.75)
    material_editor.save_changes()
    run_test(1)
    Report.result(Tests.modified_greater_than_default, default_ragdoll.bounces[0] < modified_ragdoll.bounces[1])

    # 4) Delete 'modified's material
    material_editor.delete_material("Modified")
    material_editor.save_changes()
    run_test(2)
    Report.result(
        Tests.default_equals_modified,
        lymath.Math_IsClose(default_ragdoll.bounces[2], modified_ragdoll.bounces[2], BOUNCE_TOLERANCE),
    )

    Report.info("Default hit terrain: " + str(default_ragdoll.hit_terrain))
    Report.info("Modified hit terrain: " + str(modified_ragdoll.hit_terrain))

    Report.info("Default max bounce: " + str(default_ragdoll.reached_max_bounce))
    Report.info("Modified max bouce: " + str(modified_ragdoll.reached_max_bounce))

    Report.info("Default max bounce: " + str(default_ragdoll.bounces[0]))
    Report.info("Modified max bouce: " + str(modified_ragdoll.bounces[0]))

if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(Material_LibraryCrudOperationsReflectOnRagdollBones)
