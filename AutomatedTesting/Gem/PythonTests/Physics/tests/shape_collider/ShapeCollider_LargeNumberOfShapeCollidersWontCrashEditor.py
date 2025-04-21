"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Test case ID : C19723164
Test Case Title : Verify that if we had 512 shape colliders in the level, the level does not crash

"""


# fmt: off
class Tests():
    all_entities_created = ("All 512 entities have been created", "Failed to create all 512 entities")
    game_mode_entered    = ("Entered Game Mode",                  "Failed to enter Game Mode")
    game_mode_exited     = ("Exited Game Mode",                   "Failed to exit Game Mode")
# fmt: on


def ShapeCollider_LargeNumberOfShapeCollidersWontCrashEditor():
    """
    Summary:
     Create 512 entities with shape colliders and verify stability

    Expected Behavior:
     After 512 Shape Collider entities exist, the editor should not crash or dip in FPS

    Test Steps:
     1) Load the empty level
     2) Create 512 entities with PhysX Shape Collider and Sphere Shape components
     3) Enter/Exit game mode and wait to see if editor crashes
     4) Close the editor

    Note:
     - This test file must be called from the Open 3D Engine Editor command terminal
     - Any passed and failed tests are written to the Editor.log file.
            Parsing the file or running a log_monitor are required to observe the test results.

    :return: None
    """
    # Helper Files

    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper as helper
    from editor_python_test_tools.editor_entity_utils import EditorEntity as Entity
    from consts.physics import PHYSX_SHAPE_COLLIDER

    import editor_python_test_tools.hydra_editor_utils as hydra

    # Open 3D Engine Imports
    import azlmbr.legacy.general as general

    def idle_editor_for_check():
        """
        This will be used to verify that the editor has not crashed by increasing the duration the editor is kept open
        """
        # Enter game mode
        helper.enter_game_mode(Tests.game_mode_entered)
        # Wait 60 frames
        general.idle_wait_frames(60)
        # Exit game mode
        helper.exit_game_mode(Tests.game_mode_exited)
        # Wait 60 frames more
        general.idle_wait_frames(60)

    # 1) Load the empty level
    hydra.open_base_level()

    # 2) Create 512 entities with PhysX Shape Collider and Sphere Shape components
    entity_failure = False
    for i in range(1, 513):
        # Create Entity
        entity = Entity.create_editor_entity(f"Entity_{i}")
        entity.add_component("PhysX Static Rigid Body")

        # Add components
        entity.add_component(PHYSX_SHAPE_COLLIDER)
        if i % 3 == 0:
            shape_component_name = "Capsule Shape"
        elif i % 2 == 0:
            shape_component_name = "Box Shape"
        else:
            shape_component_name = "Sphere Shape"

        entity.add_component(shape_component_name)

        # Verify the entity contains the components
        components_added = entity.has_component(PHYSX_SHAPE_COLLIDER) and entity.has_component(shape_component_name)
        if not components_added:
            entity_failure = True
            Report.info(f"Entity_{i} failed to add either PhysX Shape Collider or {shape_component_name}")
    Report.result(Tests.all_entities_created, not entity_failure)

    # 3) Enter/Exit game mode and wait to see if editor crashes
    idle_editor_for_check()


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(ShapeCollider_LargeNumberOfShapeCollidersWontCrashEditor)
