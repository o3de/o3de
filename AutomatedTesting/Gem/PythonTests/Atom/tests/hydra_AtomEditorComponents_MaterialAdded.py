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
    material_creation = (
        "Material Entity successfully created",
        "Material Entity failed to be created")
    material_component = (
        "Entity has a Material component",
        "Entity failed to find Material component")
    material_disabled = (
        "Material component disabled",
        "Material component was not disabled.")
    actor_component = (
        "Entity has an Actor component",
        "Entity did not have an Actor component")
    actor_undo = (
        "Entity Actor component gone",
        "Entity Actor component add failed to undo")
    mesh_component = (
        "Entity has a Mesh component",
        "Entity did not have a Mesh component")
    material_enabled = (
        "Material component enabled",
        "Material component was not enabled.")
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


def AtomEditorComponents_Material_AddedToEntity():
    """
    Summary:
    Tests the Material component can be added to an entity and has the expected functionality.

    Test setup:
    - Wait for Editor idle loop.
    - Open the "Base" level.

    Expected Behavior:
    The component can be added, used in game mode, hidden/shown, deleted, and has accurate required components.
    Creation and deletion undo/redo should also work.

    Test Steps:
    1) Create a Material entity with no components.
    2) Add a Material component to Material entity.
    3) UNDO the entity creation and component addition.
    4) REDO the entity creation and component addition.
    5) Verify Material component not enabled.
    6) Add Actor component since it is required by the Material component.
    7) Verify Material component is enabled.
    8) UNDO add Actor component
    9) Verify Material component not enabled.
    10) Add Mesh component since it is required by the Material component.
    11) Verify Material component is enabled.
    12) Enter/Exit game mode.
    13) Test IsHidden.
    14) Test IsVisible.
    15) Delete Material entity.
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
        TestHelper.open_level("", "Base")

        # Test steps begin.
        # 1. Create a Material entity with no components.
        material_entity = EditorEntity.create_editor_entity(AtomComponentProperties.material())
        Report.critical_result(Tests.material_creation, material_entity.exists())

        # 2. Add a Material component to Material entity.
        material_component = material_entity.add_component(AtomComponentProperties.material())
        Report.critical_result(
            Tests.material_component,
            material_entity.has_component(AtomComponentProperties.material()))

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
        Report.result(Tests.creation_undo, not material_entity.exists())

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
        Report.result(Tests.creation_redo, material_entity.exists())

        # 5. Verify Material component not enabled.
        Report.result(Tests.material_disabled, not material_component.is_enabled())

        # 6. Add Actor component since it is required by the Material component.
        material_entity.add_component(AtomComponentProperties.actor())
        Report.result(Tests.actor_component, material_entity.has_component(AtomComponentProperties.actor()))

        # 7. Verify Material component is enabled.
        Report.result(Tests.material_enabled, material_component.is_enabled())

        # 8. UNDO component addition.
        general.undo()
        general.idle_wait_frames(1)
        Report.result(Tests.actor_undo, not material_entity.has_component(AtomComponentProperties.actor()))

        # 9. Verify Material component not enabled.
        Report.result(Tests.material_disabled, not material_component.is_enabled())

        # 10. Add Mesh component since it is required by the Material component.
        material_entity.add_component(AtomComponentProperties.mesh())
        Report.result(Tests.mesh_component, material_entity.has_component(AtomComponentProperties.mesh()))

        # 11. Verify Material component is enabled.
        Report.result(Tests.material_enabled, material_component.is_enabled())

        # 12. Enter/Exit game mode.
        TestHelper.enter_game_mode(Tests.enter_game_mode)
        general.idle_wait_frames(1)
        TestHelper.exit_game_mode(Tests.exit_game_mode)

        # 13. Test IsHidden.
        material_entity.set_visibility_state(False)
        Report.result(Tests.is_hidden, material_entity.is_hidden() is True)

        # 14. Test IsVisible.
        material_entity.set_visibility_state(True)
        general.idle_wait_frames(1)
        Report.result(Tests.is_visible, material_entity.is_visible() is True)

        # 15. Delete Material entity.
        material_entity.delete()
        Report.result(Tests.entity_deleted, not material_entity.exists())

        # 16. UNDO deletion.
        general.undo()
        Report.result(Tests.deletion_undo, material_entity.exists())

        # 17. REDO deletion.
        general.redo()
        Report.result(Tests.deletion_redo, not material_entity.exists())

        # 18. Look for errors or asserts.
        TestHelper.wait_for_condition(lambda: error_tracer.has_errors or error_tracer.has_asserts, 1.0)
        for error_info in error_tracer.errors:
            Report.info(f"Error: {error_info.filename} {error_info.function} | {error_info.message}")
        for assert_info in error_tracer.asserts:
            Report.info(f"Assert: {assert_info.filename} {assert_info.function} | {assert_info.message}")


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(AtomEditorComponents_Material_AddedToEntity)
