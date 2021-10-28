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
        "Camera component was added to Camera entity",
        "Camera component failed to be added to entity")
    camera_component_check = (
        "Entity has a Camera component",
        "Entity failed to find Camera component")
    camera_property_set = (
        "DepthOfField Entity set Camera Entity",
        "DepthOfField Entity could not set Camera Entity")
    creation_undo = (
        "UNDO Entity creation success",
        "UNDO Entity creation failed")
    creation_redo = (
        "REDO Entity creation success",
        "REDO Entity creation failed")
    depth_of_field_creation = (
        "DepthOfField Entity successfully created",
        "DepthOfField Entity failed to be created")
    depth_of_field_component = (
        "Entity has a DepthOfField component",
        "Entity failed to find DepthOfField component")
    depth_of_field_disabled = (
        "DepthOfField component disabled",
        "DepthOfField component was not disabled.")
    post_fx_component = (
        "Entity has a Post FX Layer component",
        "Entity did not have a Post FX Layer component")
    depth_of_field_enabled = (
        "DepthOfField component enabled",
        "DepthOfField component was not enabled.")
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


def AtomEditorComponents_DepthOfField_AddedToEntity():
    """
    Summary:
    Tests the DepthOfField component can be added to an entity and has the expected functionality.

    Test setup:
    - Wait for Editor idle loop.
    - Open the "Base" level.

    Expected Behavior:
    The component can be added, used in game mode, hidden/shown, deleted, and has accurate required components.
    Creation and deletion undo/redo should also work.

    Test Steps:
    1) Create a DepthOfField entity with no components.
    2) Add a DepthOfField component to DepthOfField entity.
    3) UNDO the entity creation and component addition.
    4) REDO the entity creation and component addition.
    5) Verify DepthOfField component not enabled.
    6) Add Post FX Layer component since it is required by the DepthOfField component.
    7) Verify DepthOfField component is enabled.
    8) Enter/Exit game mode.
    9) Test IsHidden.
    10) Test IsVisible.
    11) Add Camera entity.
    12) Add Camera component to Camera entity.
    13) Set the DepthOfField components's Camera Entity to the newly created Camera entity.
    14) Delete DepthOfField entity.
    15) UNDO deletion.
    16) REDO deletion.
    17) Look for errors and asserts.

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
        # 1. Create a DepthOfField entity with no components.
        depth_of_field_entity = EditorEntity.create_editor_entity(AtomComponentProperties.depth_of_field())
        Report.critical_result(Tests.depth_of_field_creation, depth_of_field_entity.exists())

        # 2. Add a DepthOfField component to DepthOfField entity.
        depth_of_field_component = depth_of_field_entity.add_component(AtomComponentProperties.depth_of_field())
        Report.critical_result(Tests.depth_of_field_component,
                               depth_of_field_entity.has_component(AtomComponentProperties.depth_of_field()))

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
        Report.result(Tests.creation_undo, not depth_of_field_entity.exists())

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
        Report.result(Tests.creation_redo, depth_of_field_entity.exists())

        # 5. Verify DepthOfField component not enabled.
        Report.result(Tests.depth_of_field_disabled, not depth_of_field_component.is_enabled())

        # 6. Add Post FX Layer component since it is required by the DepthOfField component.
        depth_of_field_entity.add_component(AtomComponentProperties.postfx_layer())
        Report.result(Tests.post_fx_component, depth_of_field_entity.has_component(AtomComponentProperties.postfx_layer()))

        # 7. Verify DepthOfField component is enabled.
        Report.result(Tests.depth_of_field_enabled, depth_of_field_component.is_enabled())

        # 8. Enter/Exit game mode.
        TestHelper.enter_game_mode(Tests.enter_game_mode)
        general.idle_wait_frames(1)
        TestHelper.exit_game_mode(Tests.exit_game_mode)

        # 9. Test IsHidden.
        depth_of_field_entity.set_visibility_state(False)
        Report.result(Tests.is_hidden, depth_of_field_entity.is_hidden() is True)

        # 10. Test IsVisible.
        depth_of_field_entity.set_visibility_state(True)
        general.idle_wait_frames(1)
        Report.result(Tests.is_visible, depth_of_field_entity.is_visible() is True)

        # 11. Add Camera entity.
        camera_entity = EditorEntity.create_editor_entity(AtomComponentProperties.camera())
        Report.result(Tests.camera_creation, camera_entity.exists())

        # 12. Add Camera component to Camera entity.
        camera_entity.add_component(AtomComponentProperties.camera())
        Report.result(Tests.camera_component_added, camera_entity.has_component(AtomComponentProperties.camera()))

        # 13. Set the DepthOfField components's Camera Entity to the newly created Camera entity.
        depth_of_field_component.set_component_property_value(
            AtomComponentProperties.depth_of_field('Camera Entity'), camera_entity.id)
        Report.result(
            Tests.camera_property_set,
            camera_entity.id == depth_of_field_component.get_component_property_value(
                                    AtomComponentProperties.depth_of_field('Camera Entity')))

        # 14. Delete DepthOfField entity.
        depth_of_field_entity.delete()
        Report.result(Tests.entity_deleted, not depth_of_field_entity.exists())

        # 15. UNDO deletion.
        general.undo()
        Report.result(Tests.deletion_undo, depth_of_field_entity.exists())

        # 16. REDO deletion.
        general.redo()
        Report.result(Tests.deletion_redo, not depth_of_field_entity.exists())

        # 17. Look for errors and asserts.
        TestHelper.wait_for_condition(lambda: error_tracer.has_errors or error_tracer.has_asserts, 1.0)
        for error_info in error_tracer.errors:
            Report.info(f"Error: {error_info.filename} {error_info.function} | {error_info.message}")
        for assert_info in error_tracer.asserts:
            Report.info(f"Assert: {assert_info.filename} {assert_info.function} | {assert_info.message}")


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(AtomEditorComponents_DepthOfField_AddedToEntity)
