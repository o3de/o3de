"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

class Tests:
    postfx_gradient_weight_creation = (
        "PostFX Gradient Weight Modifier Entity successfully created",
        "P0: PostFX Gradient Weight Modifier Entity failed to be created")
    postfx_gradient_weight_component = (
        "Entity has a PostFX Gradient Weight Modifier component",
        "P0: Entity failed to find PostFX Gradient Weight Modifier component")
    postfx_gradient_weight_component_removed = (
        "Entity has a PostFX Gradient Weight Modifier component",
        "P0: Entity failed to find PostFX Gradient Weight Modifier component")
    postfx_gradient_weight_disabled = (
        "PostFX Gradient Weight Modifier component disabled",
        "P0: PostFX Gradient Weight Modifier component was not disabled.")
    postfx_layer_component = (
        "Entity has a PostFX Layer component",
        "P0: Entity did not have an PostFX Layer component")
    postfx_gradient_weight_enabled = (
        "PostFX Gradient Weight Modifier component enabled",
        "P0: PostFX Gradient Weight Modifier component was not enabled.")
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
    gradient_entity_id = (
        "Gradient Entity Id property set",
        "P1: 'Gradient Entity Id' property failed to set")
    opacity = (
        "Opacity property set",
        "P1: 'Opacity' property failed to set")
    invert_input = (
        "Invert Input property set",
        "P1: 'Invert Input' property failed to set")
    enable_levels = (
        "Enable Levels property set",
        "P1: 'Enable Levels' property failed to set")
    input_max = (
        "Input Max property set",
        "P1: 'Input Max' property failed to set")
    input_min = (
        "Input Min property set",
        "P1: 'Input Min' property failed to set")
    input_mid = (
        "Input Mid property set",
        "P1: 'Input Mid' property failed to set")
    output_max = (
        "Output Max property set",
        "P1: 'Output Max' property failed to set")
    output_min = (
        "Output Min property set",
        "P1: 'Output Min' property failed to set")
    enable_transform = (
        "Enable Transform property set",
        "P1: 'Enable Transform' property failed to set")
    scale = (
        "Scale property set",
        "P1: 'Scale' property failed to set")
    rotate = (
        "Rotate property set",
        "P1: 'Rotate' property failed to set")
    translate = (
        "Translate property set",
        "P1: 'Translate' property failed to set")


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
    3) Remove PostFX Gradient Weight Modifier component.
    4) UNDO component removal.
    5) Verify PostFX Gradient Weight Modifier component not enabled.
    6) Add PostFX Layer component since it is required by the PostFX Gradient Weight Modifier component.
    7) Verify PostFX Gradient Weight Modifier component is enabled.
    8) Set 'Gradient Entity Id' property
    9) Set 'Opacity' property
    10) Toggle 'Invert Input' property
    11) Toggle 'Enable Levels' property
    12) Set 'Input Max' property
    13) Set 'Input Min' property
    14) Set 'Input Mid' property
    15) Set 'Output Max' property
    16) Set 'Output Min' property
    17) Toggle 'Enable Transform' property
    18) Set 'Scale' property
    19) Set 'Rotate' property
    20) Set 'Translate' property
    21) Enter/Exit game mode.
    22) Test IsHidden.
    23) Test IsVisible.
    24) Delete PostFX Gradient Weight Modifier entity.
    25) UNDO deletion.
    26) REDO deletion.
    27) Look for errors.

    :return: None
    """

    import azlmbr.legacy.general as general

    from azlmbr.math import Vector3, Math_IsClose
    from editor_python_test_tools.editor_entity_utils import EditorEntity
    from editor_python_test_tools.utils import Report, Tracer, TestHelper
    from Atom.atom_utils.atom_constants import AtomComponentProperties

    with Tracer() as error_tracer:
        # Test setup begins.
        # Setup: Wait for Editor idle loop before executing Python hydra scripts then open "Base" level.
        TestHelper.init_idle()
        TestHelper.open_level("Graphics", "base_empty")

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

        # 3. Remove PostFX Gradient Weight Modifier component.
        postfx_gradient_weight_component.remove()
        general.idle_wait_frames(1)
        Report.critical_result(
            Tests.postfx_gradient_weight_component_removed,
            not postfx_gradient_weight_entity.has_component(AtomComponentProperties.postfx_gradient()))

        # 4. UNDO component removal.
        general.undo()
        general.idle_wait_frames(1)
        Report.critical_result(
            Tests.postfx_gradient_weight_component,
            postfx_gradient_weight_entity.has_component(AtomComponentProperties.postfx_gradient()))

        # 5. Verify PostFX Gradient Weight Modifier component not enabled.
        Report.result(Tests.postfx_gradient_weight_disabled, not postfx_gradient_weight_component.is_enabled())

        # 6. Add PostFX Layer component since it is required by the PostFX Gradient Weight Modifier component.
        postfx_gradient_weight_entity.add_component(AtomComponentProperties.postfx_layer())
        Report.result(
            Tests.postfx_layer_component,
            postfx_gradient_weight_entity.has_component(AtomComponentProperties.postfx_layer()))

        # 7. Verify PostFX Gradient Weight Modifier component is enabled.
        Report.result(Tests.postfx_gradient_weight_enabled, postfx_gradient_weight_component.is_enabled())

        # 8. Set 'Gradient Entity Id' property
        gradient_entity = EditorEntity.create_editor_entity('Gradient Entity')
        gradient_entity.add_components(['FastNoise Gradient', 'Gradient Transform Modifier', 'Box Shape'])
        postfx_gradient_weight_component.set_component_property_value(
            AtomComponentProperties.postfx_gradient('Gradient Entity Id'), gradient_entity.id)
        Report.result(
            Tests.gradient_entity_id,
            postfx_gradient_weight_component.get_component_property_value(
                AtomComponentProperties.postfx_gradient('Gradient Entity Id')) == gradient_entity.id)

        # 9. Set 'Opacity' property
        postfx_gradient_weight_component.set_component_property_value(
            AtomComponentProperties.postfx_gradient('Opacity'), 0.9)
        Report.result(
            Tests.opacity,
            Math_IsClose(postfx_gradient_weight_component.get_component_property_value(
                AtomComponentProperties.postfx_gradient('Opacity')), 0.9))

        # 10. Toggle 'Invert Input' property
        postfx_gradient_weight_component.set_component_property_value(
            AtomComponentProperties.postfx_gradient('Invert Input'), True)
        Report.result(
            Tests.invert_input,
            postfx_gradient_weight_component.get_component_property_value(
                AtomComponentProperties.postfx_gradient('Invert Input')) is True)

        postfx_gradient_weight_component.set_component_property_value(
            AtomComponentProperties.postfx_gradient('Invert Input'), False)
        Report.result(
            Tests.invert_input,
            postfx_gradient_weight_component.get_component_property_value(
                AtomComponentProperties.postfx_gradient('Invert Input')) is False)

        # 11. Toggle 'Enable Levels' property
        postfx_gradient_weight_component.set_component_property_value(
            AtomComponentProperties.postfx_gradient('Enable Levels'), True)
        Report.result(
            Tests.enable_levels,
            postfx_gradient_weight_component.get_component_property_value(
                AtomComponentProperties.postfx_gradient('Enable Levels')) is True)

        # 12. Set 'Input Max' property
        postfx_gradient_weight_component.set_component_property_value(
            AtomComponentProperties.postfx_gradient('Input Max'), 0.9)
        Report.result(
            Tests.input_max,
            Math_IsClose(postfx_gradient_weight_component.get_component_property_value(
                AtomComponentProperties.postfx_gradient('Input Max')), 0.9))

        # 13. Set 'Input Min' property
        postfx_gradient_weight_component.set_component_property_value(
            AtomComponentProperties.postfx_gradient('Input Min'), 0.1)
        Report.result(
            Tests.input_min,
            Math_IsClose(postfx_gradient_weight_component.get_component_property_value(
                AtomComponentProperties.postfx_gradient('Input Min')), 0.1))

        # 14. Set 'Input Mid' property
        postfx_gradient_weight_component.set_component_property_value(
            AtomComponentProperties.postfx_gradient('Input Mid'), 2.0)
        Report.result(
            Tests.input_mid,
            Math_IsClose(postfx_gradient_weight_component.get_component_property_value(
                AtomComponentProperties.postfx_gradient('Input Mid')), 2.0))

        # 15. Set 'Output Max' property
        postfx_gradient_weight_component.set_component_property_value(
            AtomComponentProperties.postfx_gradient('Output Max'), 0.9)
        Report.result(
            Tests.output_max,
            Math_IsClose(postfx_gradient_weight_component.get_component_property_value(
                AtomComponentProperties.postfx_gradient('Output Max')), 0.9))

        # 16. Set 'Output Min' property
        postfx_gradient_weight_component.set_component_property_value(
            AtomComponentProperties.postfx_gradient('Output Min'), 0.1)
        Report.result(
            Tests.output_min,
            Math_IsClose(postfx_gradient_weight_component.get_component_property_value(
                AtomComponentProperties.postfx_gradient('Output Min')), 0.1))

        # 17. Toggle 'Enable Transform' property
        postfx_gradient_weight_component.set_component_property_value(
            AtomComponentProperties.postfx_gradient('Enable Transform'), True)
        Report.result(
            Tests.enable_transform,
            postfx_gradient_weight_component.get_component_property_value(
                AtomComponentProperties.postfx_gradient('Enable Transform')) is True)

        # 18. Set 'Scale' property
        x2 = Vector3(2.0, 2.0, 2.0)
        postfx_gradient_weight_component.set_component_property_value(
            AtomComponentProperties.postfx_gradient('Scale'), x2)
        Report.result(
            Tests.scale,
            postfx_gradient_weight_component.get_component_property_value(
                AtomComponentProperties.postfx_gradient('Scale')) == x2)

        # 19. Set 'Rotate' property
        rotation = Vector3(5.0, 5.0, 5.0)
        postfx_gradient_weight_component.set_component_property_value(
           AtomComponentProperties.postfx_gradient('Rotate'), rotation)
        Report.result(
            Tests.rotate,
            postfx_gradient_weight_component.get_component_property_value(
                AtomComponentProperties.postfx_gradient('Rotate')) == rotation)

        # 20. Set 'Translate' property
        translation = Vector3(1.0, 1.0, 0.0)
        postfx_gradient_weight_component.set_component_property_value(
           AtomComponentProperties.postfx_gradient('Translate'), translation)
        Report.result(
            Tests.translate,
            postfx_gradient_weight_component.get_component_property_value(
                AtomComponentProperties.postfx_gradient('Translate')) == translation)

        # 21. Enter/Exit game mode.
        TestHelper.enter_game_mode(Tests.enter_game_mode)
        general.idle_wait_frames(1)
        TestHelper.exit_game_mode(Tests.exit_game_mode)

        # 22. Test IsHidden.
        postfx_gradient_weight_entity.set_visibility_state(False)
        Report.result(Tests.is_hidden, postfx_gradient_weight_entity.is_hidden() is True)

        # 23. Test IsVisible.
        postfx_gradient_weight_entity.set_visibility_state(True)
        general.idle_wait_frames(1)
        Report.result(Tests.is_visible, postfx_gradient_weight_entity.is_visible() is True)

        # 24. Delete PostFX Gradient Weight Modifier entity.
        postfx_gradient_weight_entity.delete()
        Report.result(Tests.entity_deleted, not postfx_gradient_weight_entity.exists())

        # 25. UNDO deletion.
        general.undo()
        general.idle_wait_frames(1)
        Report.result(Tests.deletion_undo, postfx_gradient_weight_entity.exists())

        # 26. REDO deletion.
        general.redo()
        general.idle_wait_frames(1)
        Report.result(Tests.deletion_redo, not postfx_gradient_weight_entity.exists())

        # 27. Look for errors or asserts.
        TestHelper.wait_for_condition(lambda: error_tracer.has_errors or error_tracer.has_asserts, 1.0)
        for error_info in error_tracer.errors:
            Report.info(f"Error: {error_info.filename} {error_info.function} | {error_info.message}")
        for assert_info in error_tracer.asserts:
            Report.info(f"Assert: {assert_info.filename} {assert_info.function} | {assert_info.message}")


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(AtomEditorComponents_PostFXGradientWeightModifier_AddedToEntity)
