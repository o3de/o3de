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
    deferred_fog_creation = (
        "Deferred Fog Entity successfully created",
        "Deferred Fog Entity failed to be created")
    deferred_fog_component = (
        "Entity has a Deferred Fog component",
        "Entity failed to find Deferred Fog component")
    deferred_fog_disabled = (
        "Deferred Fog component disabled",
        "Deferred Fog component was not disabled")
    postfx_layer_component = (
        "Entity has a PostFX Layer component",
        "Entity did not have an PostFX Layer component")
    deferred_fog_enabled = (
        "Deferred Fog component enabled",
        "Deferred Fog component was not enabled")
    enable_deferred_fog_parameter_enabled = (
        "Enable Deferred Fog parameter enabled",
        "Enable Deferred Fog parameter was not enabled")
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


def AtomEditorComponents_DeferredFog_AddedToEntity():
    """
    Summary:
    Tests the Deferred Fog component can be added to an entity and has the expected functionality.

    Test setup:
    - Wait for Editor idle loop.
    - Open the "Base" level.

    Expected Behavior:
    The component can be added, used in game mode, hidden/shown, deleted, and has accurate required components.
    Creation and deletion undo/redo should also work.

    Test Steps:
    1) Create an Deferred Fog entity with no components.
    2) Add Deferred Fog component to Deferred Fog entity.
    3) UNDO the entity creation and component addition.
    4) REDO the entity creation and component addition.
    5) Verify Deferred Fog component not enabled.
    6) Add PostFX Layer component since it is required by the Deferred Fog component.
    7) Verify Deferred Fog component is enabled.
    8) Enable the "Enable Deferred Fog" parameter.
    9) Enter/Exit game mode.
    10) Test IsHidden.
    11) Test IsVisible.
    12) Delete Deferred Fog entity.
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
        # 1. Create an Deferred Fog entity with no components.
        deferred_fog_entity = EditorEntity.create_editor_entity(AtomComponentProperties.deferred_fog())
        Report.critical_result(Tests.deferred_fog_creation, deferred_fog_entity.exists())

        # 2. Add Deferred Fog component to Deferred Fog entity.
        deferred_fog_component = deferred_fog_entity.add_component(
            AtomComponentProperties.deferred_fog())
        Report.critical_result(
            Tests.deferred_fog_component,
            deferred_fog_entity.has_component(AtomComponentProperties.deferred_fog()))

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
        Report.result(Tests.creation_undo, not deferred_fog_entity.exists())

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
        Report.result(Tests.creation_redo, deferred_fog_entity.exists())

        # 5. Verify Deferred Fog component not enabled.
        Report.result(Tests.deferred_fog_disabled, not deferred_fog_component.is_enabled())

        # 6. Add PostFX Layer component since it is required by the Deferred Fog component.
        deferred_fog_entity.add_component(AtomComponentProperties.postfx_layer())
        Report.result(
            Tests.postfx_layer_component,
            deferred_fog_entity.has_component(AtomComponentProperties.postfx_layer()))

        # 7. Verify Deferred Fog component is enabled.
        Report.result(Tests.deferred_fog_enabled, deferred_fog_component.is_enabled())

        # 8. Enable the "Enable Deferred Fog" parameter.
        deferred_fog_component.set_component_property_value(
            AtomComponentProperties.deferred_fog('Enable Deferred Fog'), True)
        Report.result(Tests.enable_deferred_fog_parameter_enabled,
                      deferred_fog_component.get_component_property_value(
                          AtomComponentProperties.deferred_fog('Enable Deferred Fog')) is True)

        # 9. Enter/Exit game mode.
        TestHelper.enter_game_mode(Tests.enter_game_mode)
        general.idle_wait_frames(1)
        TestHelper.exit_game_mode(Tests.exit_game_mode)

        # 10. Test IsHidden.
        deferred_fog_entity.set_visibility_state(False)
        Report.result(Tests.is_hidden, deferred_fog_entity.is_hidden() is True)

        # 11. Test IsVisible.
        deferred_fog_entity.set_visibility_state(True)
        general.idle_wait_frames(1)
        Report.result(Tests.is_visible, deferred_fog_entity.is_visible() is True)

        # 12. Delete Deferred Fog entity.
        deferred_fog_entity.delete()
        Report.result(Tests.entity_deleted, not deferred_fog_entity.exists())

        # 13. UNDO deletion.
        general.undo()
        general.idle_wait_frames(1)
        Report.result(Tests.deletion_undo, deferred_fog_entity.exists())

        # 14. REDO deletion.
        general.redo()
        general.idle_wait_frames(1)
        Report.result(Tests.deletion_redo, not deferred_fog_entity.exists())

        # 15. Look for errors and asserts.
        TestHelper.wait_for_condition(lambda: error_tracer.has_errors or error_tracer.has_asserts, 1.0)
        for error_info in error_tracer.errors:
            Report.info(f"Error: {error_info.filename} {error_info.function} | {error_info.message}")
        for assert_info in error_tracer.asserts:
            Report.info(f"Assert: {assert_info.filename} {assert_info.function} | {assert_info.message}")


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(AtomEditorComponents_DeferredFog_AddedToEntity)
