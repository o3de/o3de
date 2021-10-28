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
    postfx_gradient_weight_creation = (
        "PostFX Gradient Weight Modifier Entity successfully created",
        "PostFX Gradient Weight Modifier Entity failed to be created")
    postfx_gradient_weight_component = (
        "Entity has a PostFX Gradient Weight Modifier component",
        "Entity failed to find PostFX Gradient Weight Modifier component")
    postfx_gradient_weight_disabled = (
        "PostFX Gradient Weight Modifier component disabled",
        "PostFX Gradient Weight Modifier component was not disabled.")
    postfx_layer_component = (
        "Entity has a PostFX Layer component",
        "Entity did not have an PostFX Layer component")
    postfx_gradient_weight_enabled = (
        "PostFX Gradient Weight Modifier component enabled",
        "PostFX Gradient Weight Modifier component was not enabled.")
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


def AtomEditorComponents_PostFXGradientWeightModifier_AddedToEntity():
    """
    Summary:
    Tests the PostFX Gradient Weight Modifier component can be added to an entity and has the expected functionality.

    Test setup:
    - Wait for Editor idle loop.
    - Open the "Base" level.

    Expected Behavior:
    The component can be added, used in game mode, hidden/shown, deleted, and has accurate required components.
    Creation and deletion undo/redo should also work.

    Test Steps:
    1) Create a PostFX Gradient Weight Modifier entity with no components.
    2) Add a PostFX Gradient Weight Modifier component to PostFX Gradient Weight Modifier entity.
    3) UNDO the entity creation and component addition.
    4) REDO the entity creation and component addition.
    5) Verify PostFX Gradient Weight Modifier component not enabled.
    6) Add PostFX Layer component since it is required by the PostFX Gradient Weight Modifier component.
    7) Verify PostFX Gradient Weight Modifier component is enabled.
    8) Enter/Exit game mode.
    9) Test IsHidden.
    10) Test IsVisible.
    11) Delete PostFX Gradient Weight Modifier entity.
    12) UNDO deletion.
    13) REDO deletion.
    14) Look for errors.

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
        # 1. Create a PostFX Gradient Weight Modifier entity with no components.
        postfx_gradient_weight_entity = EditorEntity.create_editor_entity(AtomComponentProperties.postfx_gradient())
        Report.critical_result(Tests.postfx_gradient_weight_creation, postfx_gradient_weight_entity.exists())

        # 2. Add a PostFX Gradient Weight Modifier component to PostFX Gradient Weight Modifier entity.
        postfx_gradient_weight_component = postfx_gradient_weight_entity.add_component(
            AtomComponentProperties.postfx_gradient())
        Report.critical_result(
            Tests.postfx_gradient_weight_component,
            postfx_gradient_weight_entity.has_component(AtomComponentProperties.postfx_gradient()))

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
        Report.result(Tests.creation_undo, not postfx_gradient_weight_entity.exists())

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
        Report.result(Tests.creation_redo, postfx_gradient_weight_entity.exists())

        # 5. Verify PostFX Gradient Weight Modifier component not enabled.
        Report.result(Tests.postfx_gradient_weight_disabled, not postfx_gradient_weight_component.is_enabled())

        # 6. Add PostFX Layer component since it is required by the PostFX Gradient Weight Modifier component.
        postfx_gradient_weight_entity.add_component(AtomComponentProperties.postfx_layer())
        Report.result(
            Tests.postfx_layer_component,
            postfx_gradient_weight_entity.has_component(AtomComponentProperties.postfx_layer()))

        # 7. Verify PostFX Gradient Weight Modifier component is enabled.
        Report.result(Tests.postfx_gradient_weight_enabled, postfx_gradient_weight_component.is_enabled())

        # 8. Enter/Exit game mode.
        TestHelper.enter_game_mode(Tests.enter_game_mode)
        general.idle_wait_frames(1)
        TestHelper.exit_game_mode(Tests.exit_game_mode)

        # 9. Test IsHidden.
        postfx_gradient_weight_entity.set_visibility_state(False)
        Report.result(Tests.is_hidden, postfx_gradient_weight_entity.is_hidden() is True)

        # 10. Test IsVisible.
        postfx_gradient_weight_entity.set_visibility_state(True)
        general.idle_wait_frames(1)
        Report.result(Tests.is_visible, postfx_gradient_weight_entity.is_visible() is True)

        # 11. Delete PostFX Gradient Weight Modifier entity.
        postfx_gradient_weight_entity.delete()
        Report.result(Tests.entity_deleted, not postfx_gradient_weight_entity.exists())

        # 12. UNDO deletion.
        general.undo()
        Report.result(Tests.deletion_undo, postfx_gradient_weight_entity.exists())

        # 13. REDO deletion.
        general.redo()
        Report.result(Tests.deletion_redo, not postfx_gradient_weight_entity.exists())

        # 14. Look for errors or asserts.
        TestHelper.wait_for_condition(lambda: error_tracer.has_errors or error_tracer.has_asserts, 1.0)
        for error_info in error_tracer.errors:
            Report.info(f"Error: {error_info.filename} {error_info.function} | {error_info.message}")
        for assert_info in error_tracer.asserts:
            Report.info(f"Assert: {assert_info.filename} {assert_info.function} | {assert_info.message}")


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(AtomEditorComponents_PostFXGradientWeightModifier_AddedToEntity)
