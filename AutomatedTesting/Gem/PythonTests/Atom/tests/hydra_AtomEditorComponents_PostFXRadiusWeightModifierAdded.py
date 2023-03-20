"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

class Tests:
    creation_undo = (
        "UNDO Entity creation success",
        "P0: UNDO Entity creation failed")
    creation_redo = (
        "REDO Entity creation success",
        "P0: REDO Entity creation failed")
    postfx_radius_weight_creation = (
        "PostFX Radius Weight Modifier Entity successfully created",
        "P0: PostFX Radius Weight Modifier Entity failed to be created")
    postfx_radius_weight_component = (
        "Entity has a PostFX Radius Weight Modifier component",
        "P0: Entity failed to find PostFX Radius Weight Modifier component")
    postfx_radius_weight_disabled = (
        "PostFX Radius Weight Modifier component disabled",
        "P0: PostFX Radius Weight Modifier component was not disabled.")
    postfx_layer_component = (
        "Entity has a PostFX Layer component",
        "P0: Entity did not have an PostFX Layer component")
    postfx_radius_weight_enabled = (
        "PostFX Radius Weight Modifier component enabled",
        "P0: PostFX Radius Weight Modifier component was not enabled.")
    postfx_layer_component_removed = (
        "PostFX Radius Weight Modifier component was removed successfully",
        "P0: PostFX Radius Weigth Modifier component failed to be removed")
    radius = (
        "Radius property set successfully",
        "P1: Radius property failed to set")
    enter_game_mode = (
        "Entered game mode",
        "P0: Failed to enter game mode")
    exit_game_mode = (
        "Exited game mode",
        "P0: Couldn't exit game mode")
    is_visible = (
        "Entity is visible",
        "P0: Entity was not visible")
    is_hidden = (
        "Entity is hidden",
        "P0: Entity was not hidden")
    entity_deleted = (
        "Entity deleted",
        "P0: Entity was not deleted")
    deletion_undo = (
        "UNDO deletion success",
        "P0: UNDO deletion failed")
    deletion_redo = (
        "REDO deletion success",
        "P0: REDO deletion failed")


def AtomEditorComponents_PostFXRadiusWeightModifier_AddedToEntity():
    """
    Summary:
    Tests the PostFX Radius Weight Modifier component can be added to an entity and has the expected functionality.

    Test setup:
    - Wait for Editor idle loop.
    - Open the "Base" level.

    Expected Behavior:
    The component can be added, used in game mode, hidden/shown, deleted, and has accurate required components.
    Creation and deletion undo/redo should also work.

    Test Steps:
    1) Create a Post FX Radius Weight Modifier entity with no components.
    2) Add Post FX Radius Weight Modifier component to Post FX Radius Weight Modifier entity.
    3) Remove PostFX Radius Weight Modifier component.
    4) UNDO the component removal.
    5) Verify PostFX Radius Weight Modifier component not enabled.
    6) Add PostFX Layer component since it is required by the PostFX Radius Weight Modifier component.
    7) Verify PostFX Radius Weight Modifier component is enabled.
    8) Set Radius property
    9) Enter/Exit game mode.
    10) Test IsHidden.
    11) Test IsVisible.
    12) Delete PostFX Radius Weight Modifier entity.
    13) UNDO deletion.
    14) REDO deletion.
    15) Look for errors.

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
        TestHelper.open_level("Graphics", "base_empty")

        # Test steps begin.
        # 1. Create a Post FX Radius Weight Modifier entity with no components.
        postfx_radius_weight_entity = EditorEntity.create_editor_entity(AtomComponentProperties.postfx_radius())
        Report.critical_result(Tests.postfx_radius_weight_creation, postfx_radius_weight_entity.exists())

        # 2. Add Post FX Radius Weight Modifier component to Post FX Radius Weight Modifier entity.
        postfx_radius_component = postfx_radius_weight_entity.add_component(AtomComponentProperties.postfx_radius())
        Report.critical_result(
            Tests.postfx_radius_weight_component,
            postfx_radius_weight_entity.has_component(AtomComponentProperties.postfx_radius()))

        # 3. Remove PostFX Radius Weight Modifier component
        postfx_radius_component.remove()
        general.idle_wait_frames(1)
        Report.result(
            Tests.postfx_layer_component_removed,
            not postfx_radius_weight_entity.has_component(AtomComponentProperties.postfx_radius()))

        # 4. UNDO the component removal.
        general.undo()
        general.idle_wait_frames(1)
        Report.critical_result(
            Tests.postfx_radius_weight_component,
            postfx_radius_weight_entity.has_component(AtomComponentProperties.postfx_radius()))

        # 5. Verify PostFX Radius Weight Modifier component not enabled.
        Report.result(Tests.postfx_radius_weight_disabled, not postfx_radius_component.is_enabled())

        # 6. Add PostFX Layer component since it is required by the PostFX Radius Weight Modifier component.
        postfx_radius_weight_entity.add_component(AtomComponentProperties.postfx_layer())
        Report.result(
            Tests.postfx_layer_component,
            postfx_radius_weight_entity.has_component(AtomComponentProperties.postfx_layer()))

        # 7. Verify PostFX Radius Weight Modifier component is enabled.
        Report.result(Tests.postfx_radius_weight_enabled, postfx_radius_component.is_enabled())

        # 8. Set Radius property
        postfx_radius_component.set_component_property_value(AtomComponentProperties.postfx_radius('Radius'), 100.0)
        Report.result(
            Tests.radius,
            postfx_radius_component.get_component_property_value(
                AtomComponentProperties.postfx_radius('Radius')) == 100.0)

        # 9. Enter/Exit game mode.
        TestHelper.enter_game_mode(Tests.enter_game_mode)
        general.idle_wait_frames(1)
        TestHelper.exit_game_mode(Tests.exit_game_mode)

        # 10. Test IsHidden.
        postfx_radius_weight_entity.set_visibility_state(False)
        Report.result(Tests.is_hidden, postfx_radius_weight_entity.is_hidden() is True)

        # 11. Test IsVisible.
        postfx_radius_weight_entity.set_visibility_state(True)
        general.idle_wait_frames(1)
        Report.result(Tests.is_visible, postfx_radius_weight_entity.is_visible() is True)

        # 12. Delete PostFX Radius Weight Modifier entity.
        postfx_radius_weight_entity.delete()
        Report.result(Tests.entity_deleted, not postfx_radius_weight_entity.exists())

        # 13. UNDO deletion.
        general.undo()
        general.idle_wait_frames(1)
        Report.result(Tests.deletion_undo, postfx_radius_weight_entity.exists())

        # 14. REDO deletion.
        general.redo()
        general.idle_wait_frames(1)
        Report.result(Tests.deletion_redo, not postfx_radius_weight_entity.exists())

        # 15. Look for errors and asserts.
        TestHelper.wait_for_condition(lambda: error_tracer.has_errors or error_tracer.has_asserts, 1.0)
        for error_info in error_tracer.errors:
            Report.info(f"Error: {error_info.filename} {error_info.function} | {error_info.message}")
        for assert_info in error_tracer.asserts:
            Report.info(f"Assert: {assert_info.filename} {assert_info.function} | {assert_info.message}")


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(AtomEditorComponents_PostFXRadiusWeightModifier_AddedToEntity)
