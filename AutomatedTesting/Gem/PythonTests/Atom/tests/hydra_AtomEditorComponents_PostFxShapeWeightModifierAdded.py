"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

class Tests:
    postfx_shape_weight_creation = (
        "PostFx Shape Weight Modifier Entity successfully created",
        "P0: PostFx Shape Weight Modifier Entity failed to be created")
    postfx_shape_weight_component = (
        "Entity has a PostFx Shape Weight Modifier component",
        "P0: Entity failed to find PostFx Shape Weight Modifier component")
    postfx_shape_weight_component_removed = (
        "PostFx Shape Weight Modifier component removed",
        "P0: PostFx Shape Weight Modifier component failed to be removed")
    postfx_shape_weight_disabled = (
        "PostFx Shape Weight Modifier component disabled",
        "P0: PostFx Shape Weight Modifier component was not disabled.")
    postfx_layer_component = (
        "Entity has a PostFX Layer component",
        "P0: Entity did not have an PostFX Layer component")
    tube_shape_component = (
        "Entity has a Tube Shape component",
        "P0: Entity did not have a Tube Shape component")
    postfx_shape_weight_enabled = (
        "PostFx Shape Weight Modifier component enabled",
        "P0: PostFx Shape Weight Modifier component was not enabled.")
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
    fall_off = (
        "'Fall-off Distance' property set",
        "P1: 'Fall-off Distance' property failed value set."
    )


def AtomEditorComponents_postfx_shape_weight_AddedToEntity():
    """
    Summary:
    Tests the PostFx Shape Weight Modifier component can be added to an entity and has the expected functionality.

    Test setup:
    - Wait for Editor idle loop.
    - Open the "Base" level.

    Expected Behavior:
    The component can be added, used in game mode, hidden/shown, deleted, and has accurate required components.
    Creation and deletion undo/redo should also work.

    Test Steps:
    1) Create a PostFx Shape Weight Modifier entity with no components.
    2) Add a PostFx Shape Weight Modifier component to PostFx Shape Weight Modifier entity.
    3) Remove PostFX Shape Weight Modifier component.
    4) UNDO component removal.
    5) Verify PostFx Shape Weight Modifier component not enabled.
    6) Add PostFX Layer component since it is required by the PostFx Shape Weight Modifier component.
    7) Verify PostFx Shape Weight Modifier component is NOT enabled since it also requires a shape.
    8) Add a required shape looping over a list and checking if it enables PostFX Shape Weight Modifier.
    9) Undo to remove each added shape and verify PostFX Shape Weight Modifier is not enabled.
    10) Verify PostFx Shape Weight Modifier component is enabled by adding Spline and Tube Shape component.
    11) Set 'Fall-off Distance' property
    12) Enter/Exit game mode.
    13) Test IsHidden.
    14) Test IsVisible.
    15) Delete PostFx Shape Weight Modifier entity.
    16) UNDO deletion.
    17) REDO deletion.
    18) Look for errors.

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
        # 1. Create a PostFx Shape Weight Modifier entity with no components.
        postfx_shape_weight_entity = EditorEntity.create_editor_entity(AtomComponentProperties.postfx_shape())
        Report.critical_result(Tests.postfx_shape_weight_creation, postfx_shape_weight_entity.exists())

        # 2. Add a PostFx Shape Weight Modifier component to PostFx Shape Weight Modifier entity.
        postfx_shape_weight_component = postfx_shape_weight_entity.add_component(AtomComponentProperties.postfx_shape())
        Report.critical_result(
            Tests.postfx_shape_weight_component,
            postfx_shape_weight_entity.has_component(AtomComponentProperties.postfx_shape()))

        # 3. Remove PostFX Shape Weight Modifier component.
        postfx_shape_weight_component.remove()
        general.idle_wait_frames(1)
        Report.critical_result(
            Tests.postfx_shape_weight_component_removed,
            not postfx_shape_weight_entity.has_component(AtomComponentProperties.postfx_shape()))

        # 4. UNDO component removal.
        general.undo()
        general.idle_wait_frames(1)
        Report.critical_result(
            Tests.postfx_shape_weight_component,
            postfx_shape_weight_entity.has_component(AtomComponentProperties.postfx_shape()))

        # 5. Verify PostFx Shape Weight Modifier component not enabled.
        Report.result(Tests.postfx_shape_weight_disabled, not postfx_shape_weight_component.is_enabled())

        # 6. Add PostFX Layer component since it is required by the PostFx Shape Weight Modifier component.
        postfx_shape_weight_entity.add_component(AtomComponentProperties.postfx_layer())
        Report.result(
            Tests.postfx_layer_component,
            postfx_shape_weight_entity.has_component(AtomComponentProperties.postfx_layer()))

        # 7. Verify PostFx Shape Weight Modifier component is NOT enabled since it also requires a shape.
        Report.result(Tests.postfx_shape_weight_disabled, not postfx_shape_weight_component.is_enabled())

        # 8. Add a required shape looping over a list and checking if it enables PostFX Shape Weight Modifier.
        for shape in AtomComponentProperties.postfx_shape('shapes'):
            postfx_shape_weight_entity.add_component(shape)
            test_shape = (
                f"Entity has a {shape} component",
                f"Entity did not have a {shape} component")
            Report.result(test_shape, postfx_shape_weight_entity.has_component(shape))

            # Check if required shape allows PostFX Shape Weight Modifier to be enabled
            Report.result(Tests.postfx_shape_weight_enabled, postfx_shape_weight_component.is_enabled())

            # 9. Undo to remove each added shape and verify PostFX Shape Weight Modifier is not enabled.
            general.undo()
            TestHelper.wait_for_condition(lambda: not postfx_shape_weight_entity.has_component(shape), 1.0)
            Report.result(Tests.postfx_shape_weight_disabled, not postfx_shape_weight_component.is_enabled())

        # 10. Verify PostFx Shape Weight Modifier component is enabled by adding Spline and Tube Shape component.
        postfx_shape_weight_entity.add_components(['Spline', 'Tube Shape'])
        Report.result(Tests.tube_shape_component, postfx_shape_weight_entity.has_component('Tube Shape'))
        Report.result(Tests.postfx_shape_weight_enabled, postfx_shape_weight_component.is_enabled())

        # 11. Set 'Fall-off Distance' property
        postfx_shape_weight_component.set_component_property_value(
            AtomComponentProperties.postfx_shape('Fall-off Distance'), 100.0)
        Report.result(
            Tests.fall_off,
            postfx_shape_weight_component.get_component_property_value(
                AtomComponentProperties.postfx_shape('Fall-off Distance')) == 100.0)

        # 12. Enter/Exit game mode.
        TestHelper.enter_game_mode(Tests.enter_game_mode)
        general.idle_wait_frames(1)
        TestHelper.exit_game_mode(Tests.exit_game_mode)

        # 13. Test IsHidden.
        postfx_shape_weight_entity.set_visibility_state(False)
        Report.result(Tests.is_hidden, postfx_shape_weight_entity.is_hidden() is True)

        # 14. Test IsVisible.
        postfx_shape_weight_entity.set_visibility_state(True)
        general.idle_wait_frames(1)
        Report.result(Tests.is_visible, postfx_shape_weight_entity.is_visible() is True)

        # 15. Delete PostFx Shape Weight Modifier entity.
        postfx_shape_weight_entity.delete()
        Report.result(Tests.entity_deleted, not postfx_shape_weight_entity.exists())

        # 16. UNDO deletion.
        general.undo()
        general.idle_wait_frames(1)
        Report.result(Tests.deletion_undo, postfx_shape_weight_entity.exists())

        # 17. REDO deletion.
        general.redo()
        general.idle_wait_frames(1)
        Report.result(Tests.deletion_redo, not postfx_shape_weight_entity.exists())

        # 18. Look for errors or asserts.
        TestHelper.wait_for_condition(lambda: error_tracer.has_errors or error_tracer.has_asserts, 1.0)
        for error_info in error_tracer.errors:
            Report.info(f"Error: {error_info.filename} {error_info.function} | {error_info.message}")
        for assert_info in error_tracer.asserts:
            Report.info(f"Assert: {assert_info.filename} {assert_info.function} | {assert_info.message}")


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(AtomEditorComponents_postfx_shape_weight_AddedToEntity)
