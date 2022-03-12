"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

class Tests:
    camera_creation = (
        "Camera Entity successfully created",
        "Camera Entity failed to be created")
    camera_component_added = (
        "Camera component was added to entity",
        "Camera component failed to be added to entity")
    camera_component_check = (
        "Entity has a Camera component",
        "Entity failed to find Camera component")
    creation_undo = (
        "UNDO Entity creation success",
        "UNDO Entity creation failed")
    creation_redo = (
        "REDO Entity creation success",
        "REDO Entity creation failed")
    directional_light_creation = (
        "Directional Light Entity successfully created",
        "Directional Light Entity failed to be created")
    directional_light_component = (
        "Entity has a Directional Light component",
        "Entity failed to find Directional Light component")
    shadow_camera_check = (
        "Directional Light component Shadow camera set",
        "Directional Light component Shadow camera was not set")
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


def AtomEditorComponents_DirectionalLight_AddedToEntity():
    """
    Summary:
    Tests the Directional Light component can be added to an entity and has the expected functionality.

    Test setup:
    - Wait for Editor idle loop.
    - Open the "Base" level.

    Expected Behavior:
    The component can be added, used in game mode, hidden/shown, deleted, and has accurate required components.
    Creation and deletion undo/redo should also work.

    Test Steps:
    1) Create a Directional Light entity with no components.
    2) Add Directional Light component to Directional Light entity.
    3) UNDO the entity creation and component addition.
    4) REDO the entity creation and component addition.
    5) Enter/Exit game mode.
    6) Test IsHidden.
    7) Test IsVisible.
    8) Add Camera entity.
    9) Add Camera component to Camera entity
    10) Set the Directional Light component property Shadow|Camera to the Camera entity.
    11) Delete Directional Light entity.
    12) UNDO deletion.
    13) REDO deletion.
    14) Look for errors and asserts.

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
        # 1. Create a Directional Light entity with no components.
        directional_light_entity = EditorEntity.create_editor_entity(AtomComponentProperties.directional_light())
        Report.critical_result(Tests.directional_light_creation, directional_light_entity.exists())

        # 2. Add Directional Light component to Directional Light entity.
        directional_light_component = directional_light_entity.add_component(AtomComponentProperties.directional_light())
        Report.critical_result(
            Tests.directional_light_component,
            directional_light_entity.has_component(AtomComponentProperties.directional_light()))

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
        Report.result(Tests.creation_undo, not directional_light_entity.exists())

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
        Report.result(Tests.creation_redo, directional_light_entity.exists())

        # 5. Enter/Exit game mode.
        TestHelper.enter_game_mode(Tests.enter_game_mode)
        general.idle_wait_frames(1)
        TestHelper.exit_game_mode(Tests.exit_game_mode)

        # 6. Test IsHidden.
        directional_light_entity.set_visibility_state(False)
        Report.result(Tests.is_hidden, directional_light_entity.is_hidden() is True)

        # 7. Test IsVisible.
        directional_light_entity.set_visibility_state(True)
        general.idle_wait_frames(1)
        Report.result(Tests.is_visible, directional_light_entity.is_visible() is True)

        # 8. Add Camera entity.
        camera_entity = EditorEntity.create_editor_entity(AtomComponentProperties.camera())
        Report.result(Tests.camera_creation, camera_entity.exists())

        # 9. Add Camera component to Camera entity.
        camera_entity.add_component(AtomComponentProperties.camera())
        Report.result(Tests.camera_component_added, camera_entity.has_component(AtomComponentProperties.camera()))

        # 10. Set the Directional Light component property Shadow|Camera to the Camera entity.
        directional_light_component.set_component_property_value(
            AtomComponentProperties.directional_light('Camera'), camera_entity.id)
        Report.result(
            Tests.shadow_camera_check,
            camera_entity.id == directional_light_component.get_component_property_value(
                                    AtomComponentProperties.directional_light('Camera')))

        # 11. Delete DirectionalLight entity.
        directional_light_entity.delete()
        Report.result(Tests.entity_deleted, not directional_light_entity.exists())

        # 12. UNDO deletion.
        general.undo()
        Report.result(Tests.deletion_undo, directional_light_entity.exists())

        # 13. REDO deletion.
        general.redo()
        Report.result(Tests.deletion_redo, not directional_light_entity.exists())

        # 14. Look for errors and asserts.
        TestHelper.wait_for_condition(lambda: error_tracer.has_errors or error_tracer.has_asserts, 1.0)
        for error_info in error_tracer.errors:
            Report.info(f"Error: {error_info.filename} {error_info.function} | {error_info.message}")
        for assert_info in error_tracer.asserts:
            Report.info(f"Assert: {assert_info.filename} {assert_info.function} | {assert_info.message}")


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(AtomEditorComponents_DirectionalLight_AddedToEntity)
