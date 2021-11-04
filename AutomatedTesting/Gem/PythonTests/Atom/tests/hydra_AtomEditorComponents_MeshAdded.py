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
    mesh_entity_creation = (
        "Mesh Entity successfully created",
        "Mesh Entity failed to be created")
    mesh_component_added = (
        "Entity has a Mesh component",
        "Entity failed to find Mesh component")
    mesh_asset_specified = (
        "Mesh asset set",
        "Mesh asset not set")
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


def AtomEditorComponents_Mesh_AddedToEntity():
    """
    Summary:
    Tests the Mesh component can be added to an entity and has the expected functionality.

    Test setup:
    - Wait for Editor idle loop.
    - Open the "Base" level.

    Expected Behavior:
    The component can be added, used in game mode, hidden/shown, deleted, and has accurate required components.
    Creation and deletion undo/redo should also work.

    Test Steps:
    1) Create a Mesh entity with no components.
    2) Add a Mesh component to Mesh entity.
    3) UNDO the entity creation and component addition.
    4) REDO the entity creation and component addition.
    5) Specify the Mesh component asset
    6) Enter/Exit game mode.
    7) Test IsHidden.
    8) Test IsVisible.
    9) Delete Mesh entity.
    10) UNDO deletion.
    11) REDO deletion.
    12) Look for errors.

    :return: None
    """

    import os

    import azlmbr.legacy.general as general

    from editor_python_test_tools.asset_utils import Asset
    from editor_python_test_tools.editor_entity_utils import EditorEntity
    from editor_python_test_tools.utils import Report, Tracer, TestHelper
    from Atom.atom_utils.atom_constants import AtomComponentProperties

    with Tracer() as error_tracer:
        # Test setup begins.
        # Setup: Wait for Editor idle loop before executing Python hydra scripts then open "Base" level.
        TestHelper.init_idle()
        TestHelper.open_level("", "Base")

        # Test steps begin.
        # 1. Create a Mesh entity with no components.
        mesh_entity = EditorEntity.create_editor_entity(AtomComponentProperties.mesh())
        Report.critical_result(Tests.mesh_entity_creation, mesh_entity.exists())

        # 2. Add a Mesh component to Mesh entity.
        mesh_component = mesh_entity.add_component(AtomComponentProperties.mesh())
        Report.critical_result(
            Tests.mesh_component_added,
            mesh_entity.has_component(AtomComponentProperties.mesh()))

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
        Report.result(Tests.creation_undo, not mesh_entity.exists())

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
        Report.result(Tests.creation_redo, mesh_entity.exists())

        # 5. Set Mesh component asset property
        model_path = os.path.join('Objects', 'shaderball', 'shaderball_default_1m.azmodel')
        model = Asset.find_asset_by_path(model_path)
        mesh_component.set_component_property_value(AtomComponentProperties.mesh('Mesh Asset'), model.id)
        Report.result(Tests.mesh_asset_specified,
                      mesh_component.get_component_property_value(AtomComponentProperties.mesh('Mesh Asset')) == model.id)

        # 6. Enter/Exit game mode.
        TestHelper.enter_game_mode(Tests.enter_game_mode)
        general.idle_wait_frames(1)
        TestHelper.exit_game_mode(Tests.exit_game_mode)

        # 7. Test IsHidden.
        mesh_entity.set_visibility_state(False)
        Report.result(Tests.is_hidden, mesh_entity.is_hidden() is True)

        # 8. Test IsVisible.
        mesh_entity.set_visibility_state(True)
        general.idle_wait_frames(1)
        Report.result(Tests.is_visible, mesh_entity.is_visible() is True)

        # 9. Delete Mesh entity.
        mesh_entity.delete()
        Report.result(Tests.entity_deleted, not mesh_entity.exists())

        # 10. UNDO deletion.
        general.undo()
        Report.result(Tests.deletion_undo, mesh_entity.exists())

        # 11. REDO deletion.
        general.redo()
        Report.result(Tests.deletion_redo, not mesh_entity.exists())

        # 12. Look for errors or asserts.
        TestHelper.wait_for_condition(lambda: error_tracer.has_errors or error_tracer.has_asserts, 1.0)
        for error_info in error_tracer.errors:
            Report.info(f"Error: {error_info.filename} {error_info.function} | {error_info.message}")
        for assert_info in error_tracer.asserts:
            Report.info(f"Assert: {assert_info.filename} {assert_info.function} | {assert_info.message}")


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(AtomEditorComponents_Mesh_AddedToEntity)
