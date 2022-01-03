"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

class Tests:
    creation_undo = (
        "UNDO Entity creation success",
        "UNDO Entity creation failed")
    creation_redo = (
        "REDO Entity creation success",
        "REDO Entity creation failed")
    postfx_layer_entity_creation = (
        "PostFX Layer Entity successfully created",
        "PostFX Layer Entity failed to be created")
    postfx_layer_component_added = (
        "Entity has a PostFX Layer component",
        "Entity failed to find PostFX Layer component")
    enter_game_mode = (
        "Entered game mode",
        "Failed to enter game mode")
    exit_game_mode = (
        "Exited game mode",
        "Couldn't exit game mode")
    is_visible = (
        "Entity is visible",
        "Entity was not visible")
    is_hidden = (
        "Entity is hidden",
        "Entity was not hidden")
    entity_deleted = (
        "Entity deleted",
        "Entity was not deleted")
    deletion_undo = (
        "UNDO deletion success",
        "UNDO deletion failed")
    deletion_redo = (
        "REDO deletion success",
        "REDO deletion failed")


def AtomEditorComponents_postfx_layer_AddedToEntity():
    """
    Summary:
    Tests the PostFX Layer component can be added to an entity and has the expected functionality.

    Test setup:
    - Wait for Editor idle loop.
    - Open the "Base" level.

    Expected Behavior:
    The component can be added, used in game mode, hidden/shown, deleted, and has accurate required components.
    Creation and deletion undo/redo should also work.

    Test Steps:
    1) Create a PostFX Layer entity with no components.
    2) Add a PostFX Layer component to PostFX Layer entity.
    3) UNDO the entity creation and component addition.
    4) REDO the entity creation and component addition.
    5) Enter/Exit game mode.
    6) Test IsHidden.
    7) Test IsVisible.
    8) Delete PostFX Layer entity.
    9) UNDO deletion.
    10) REDO deletion.
    11) Look for errors.

    :return: None
    """

    import azlmbr.legacy.general as general

    from editor_python_test_tools.editor_entity_utils import EditorEntity
    from editor_python_test_tools.utils import Report, Tracer, TestHelper
    from Atom.atom_utils.atom_constants import AtomComponentProperties

    with Tracer() as error_tracer:
        # Test setup begins.
        # Setup: Wait for Editor idle loop before executing Python hydra scripts then open "Base" level.
        TestHelper.init_idle()
        TestHelper.open_level("", "Base")

        # Test steps begin.
        # 1. Create a PostFX Layer entity with no components.
        postfx_layer_entity = EditorEntity.create_editor_entity(AtomComponentProperties.postfx_layer())
        Report.critical_result(Tests.postfx_layer_entity_creation, postfx_layer_entity.exists())

        # 2. Add a PostFX Layer component to PostFX Layer entity.
        postfx_layer_component = postfx_layer_entity.add_component(AtomComponentProperties.postfx_layer())
        Report.critical_result(
            Tests.postfx_layer_component_added,
            postfx_layer_entity.has_component(AtomComponentProperties.postfx_layer()))

        # 3. UNDO the entity creation and component addition.
        # -> UNDO component addition.
        general.undo()
        # -> UNDO naming entity.
        general.undo()
        # -> UNDO selecting entity.
        general.undo()
        # -> UNDO entity creation.
        general.undo()
        general.idle_wait_frames(1)
        Report.result(Tests.creation_undo, not postfx_layer_entity.exists())

        # 4. REDO the entity creation and component addition.
        # -> REDO entity creation.
        general.redo()
        # -> REDO selecting entity.
        general.redo()
        # -> REDO naming entity.
        general.redo()
        # -> REDO component addition.
        general.redo()
        general.idle_wait_frames(1)
        Report.result(Tests.creation_redo, postfx_layer_entity.exists())

        # 5. Enter/Exit game mode.
        TestHelper.enter_game_mode(Tests.enter_game_mode)
        general.idle_wait_frames(1)
        TestHelper.exit_game_mode(Tests.exit_game_mode)

        # 6. Test IsHidden.
        postfx_layer_entity.set_visibility_state(False)
        Report.result(Tests.is_hidden, postfx_layer_entity.is_hidden() is True)

        # 7. Test IsVisible.
        postfx_layer_entity.set_visibility_state(True)
        general.idle_wait_frames(1)
        Report.result(Tests.is_visible, postfx_layer_entity.is_visible() is True)

        # 8. Delete PostFX Layer entity.
        postfx_layer_entity.delete()
        Report.result(Tests.entity_deleted, not postfx_layer_entity.exists())

        # 9. UNDO deletion.
        general.undo()
        Report.result(Tests.deletion_undo, postfx_layer_entity.exists())

        # 10. REDO deletion.
        general.redo()
        Report.result(Tests.deletion_redo, not postfx_layer_entity.exists())

        # 11. Look for errors or asserts.
        TestHelper.wait_for_condition(lambda: error_tracer.has_errors or error_tracer.has_asserts, 1.0)
        for error_info in error_tracer.errors:
            Report.info(f"Error: {error_info.filename} {error_info.function} | {error_info.message}")
        for assert_info in error_tracer.asserts:
            Report.info(f"Assert: {assert_info.filename} {assert_info.function} | {assert_info.message}")


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(AtomEditorComponents_postfx_layer_AddedToEntity)
