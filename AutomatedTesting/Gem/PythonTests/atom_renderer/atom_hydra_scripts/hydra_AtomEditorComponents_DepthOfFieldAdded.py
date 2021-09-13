"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

# fmt: off
class Tests:
    camera_creation = ("camera_entity Entity successfully created", "camera_entity Entity failed to be created")
    camera_component_added = ("Camera component was added to entity", "Camera component failed to be added to entity")
    camera_component_check = ("Entity has a Camera component", "Entity failed to find Camera component")
    creation_undo = ("UNDO Entity creation success", "UNDO Entity creation failed")
    creation_redo = ("REDO Entity creation success", "REDO Entity creation failed")
    depth_of_field_creation = ("DepthOfField Entity successfully created", "DepthOfField Entity failed to be created")
    depth_of_field_component = ("Entity has a DepthOfField component", "Entity failed to find DepthOfField component")
    post_fx_component = ("Entity has a Post FX Layer component", "Entity did not have a Post FX Layer component")
    enter_game_mode = ("Entered game mode", "Failed to enter game mode")
    exit_game_mode = ("Exited game mode", "Couldn't exit game mode")
    is_visible = ("Entity is visible", "Entity was not visible")
    is_hidden = ("Entity is hidden", "Entity was not hidden")
    entity_deleted = ("Entity deleted", "Entity was not deleted")
    deletion_undo = ("UNDO deletion success", "UNDO deletion failed")
    deletion_redo = ("REDO deletion success", "REDO deletion failed")
    no_error_occurred = ("No errors detected", "Errors were detected")
# fmt: on


def AtomEditorComponents_DepthOfField_AddedToEntity():
    """
    Summary:
    Tests the DepthOfField component can be added to an entity and has the expected functionality.
    First it will setup the test by deleting existing entities and creating a camera entity with camera component.
    After that it follows the test steps for pass/fail.

    Expected Behavior:
    The component can be added, used in game mode, hidden/shown, deleted, and has accurate required components.
    Creation and deletion undo/redo should also work.

    Test Steps:
    1) Creation of entity with DepthOfField component.
    2) UNDO the entity creation.
    3) REDO the entity creation.
    4) Enter/Exit game mode.
    5) Hide test.
    6) Visible test.
    7) Add Post FX Layer component.
    8) Add Camera component
    9) Delete DepthOfField entity.
    10) UNDO deletion.
    11) REDO deletion.
    12) Look for errors.

    :return: None
    """
    import os

    import azlmbr.bus as bus
    import azlmbr.entity as entity
    import azlmbr.editor as editor
    import azlmbr.legacy.general as general
    import azlmbr.math as math
    import azlmbr.paths

    from editor_python_test_tools import hydra_editor_utils as hydra
    from editor_python_test_tools.editor_entity_utils import EditorEntity
    from editor_python_test_tools.utils import Report, Tracer, TestHelper as helper

    with Tracer() as error_tracer:
        # Wait for Editor idle loop before executing Python hydra scripts.
        helper.init_idle()
        helper.open_level(os.path.join(azlmbr.paths.devassets, "Levels"), "Base")

        # Delete all existing entities initially for test setup.
        search_filter = azlmbr.entity.SearchFilter()
        all_entities = entity.SearchBus(azlmbr.bus.Broadcast, "SearchEntities", search_filter)
        editor.ToolsApplicationRequestBus(bus.Broadcast, "DeleteEntities", all_entities)

        # Set up camera entity and component.
        camera_entity = EditorEntity.create_editor_entity_at(math.Vector3(512.0, 512.0, 34.0), "camera_entity")
        camera_entity.add_component("Camera")
        Report.result(Tests.camera_creation, camera_entity.id.IsValid())

        # 1. Creation of entity with DepthOfField component.
        depth_of_field = "DepthOfField"
        depth_of_field_entity = EditorEntity.create_editor_entity_at(
            math.Vector3(512.0, 512.0, 34.0), f"{depth_of_field}")
        depth_of_field_entity.add_component(depth_of_field)
        Report.result(Tests.depth_of_field_creation, depth_of_field_entity.id.IsValid())
        Report.critical_result(Tests.depth_of_field_component, depth_of_field_entity.has_component(depth_of_field))

        # 2. UNDO the entity creation.
        general.undo()
        Report.result(Tests.creation_undo, len(hydra.find_entity_by_name(depth_of_field_entity)) == 0)

        # 3. REDO the entity creation.
        general.redo()
        Report.result(Tests.creation_redo, len(hydra.find_entity_by_name(depth_of_field_entity)) == 1)

        # 4. Enter/Exit game mode.
        helper.enter_game_mode(Tests.enter_game_mode)
        general.idle_wait_frames(1)
        helper.exit_game_mode(Tests.exit_game_mode)

        # 5. Hide test.
        depth_of_field_entity.set_visibility_state(False)
        is_hidden = editor.EditorEntityInfoRequestBus(bus.Event, 'IsHidden', depth_of_field_entity.id)
        Report.result(Tests.is_hidden, is_hidden is True)

        # 6. Visible test.
        depth_of_field_entity.set_visibility_state(True)
        is_visible = editor.EditorEntityInfoRequestBus(bus.Event, 'IsVisible', depth_of_field_entity.id)
        Report.result(Tests.is_visible, is_visible is True)

        # 7. Add Post FX Layer component.
        depth_of_field_entity.add_component("PostFX Layer")
        post_fx_component_added = editor.EditorComponentAPIBus(
            bus.Broadcast, "IsComponentEnabled", depth_of_field_entity.components[0])
        Report.result(Tests.post_fx_component, post_fx_component_added)

        # 8. Add camera component.
        camera_component_added = depth_of_field_entity.get_set_test(
            0, "Controller|Configuration|Camera Entity", camera_entity.id)
        Report.result(Tests.camera_component_added, camera_component_added)

        # 9. Delete DepthOfField entity.
        depth_of_field_entity.delete()
        Report.result(Tests.entity_deleted, len(hydra.find_entity_by_name(depth_of_field_entity)) == 0)

        # 10. UNDO deletion.
        general.undo()
        Report.result(Tests.deletion_undo, len(hydra.find_entity_by_name(depth_of_field_entity)) == 1)

        # 11. REDO deletion.
        general.redo()
        Report.result(Tests.deletion_redo, len(hydra.find_entity_by_name(depth_of_field_entity)) == 0)

        # 12. Look for errors.
        helper.wait_for_condition(lambda: error_tracer.has_errors, 1.0)
        Report.result(Tests.no_error_occurred, not error_tracer.has_errors)


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(AtomEditorComponents_DepthOfField_AddedToEntity)
