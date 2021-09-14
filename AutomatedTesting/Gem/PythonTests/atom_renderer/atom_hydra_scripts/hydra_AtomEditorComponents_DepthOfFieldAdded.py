"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

# fmt: off
class Tests:
    camera_creation =           ("Camera Entity successfully created",            "Camera Entity failed to be created")
    camera_component_added =    ("Camera component was added to Camera entity",   "Camera component failed to be added to entity")
    camera_component_check =    ("Entity has a Camera component",                 "Entity failed to find Camera component")
    camera_property_set =       ("DepthOfField Entity set Camera Entity",         "DepthOfField Entity could not set Camera Entity")
    creation_undo =             ("UNDO Entity creation success",                  "UNDO Entity creation failed")
    creation_redo =             ("REDO Entity creation success",                  "REDO Entity creation failed")
    depth_of_field_creation =   ("DepthOfField Entity successfully created",      "DepthOfField Entity failed to be created")
    depth_of_field_component =  ("Entity has a DepthOfField component",           "Entity failed to find DepthOfField component")
    post_fx_component =         ("Entity has a Post FX Layer component",          "Entity did not have a Post FX Layer component")
    enter_game_mode =           ("Entered game mode",                             "Failed to enter game mode")
    exit_game_mode =            ("Exited game mode",                              "Couldn't exit game mode")
    is_visible =                ("Entity is visible",                             "Entity was not visible")
    is_hidden =                 ("Entity is hidden",                              "Entity was not hidden")
    entity_deleted =            ("Entity deleted",                                "Entity was not deleted")
    deletion_undo =             ("UNDO deletion success",                         "UNDO deletion failed")
    deletion_redo =             ("REDO deletion success",                         "REDO deletion failed")
    no_error_occurred =         ("No errors detected",                            "Errors were detected")
# fmt: on


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
    5) Add Post FX Layer component since it is required by the DepthOfField component.
    6) Enter/Exit game mode.
    7) Test IsHidden.
    8) Test IsVisible.
    9) Add Camera entity.
    10) Add Camera component to Camera entity.
    11) Set the DepthOfField components's Camera Entity to the newly created Camera entity.
    12) Delete DepthOfField entity.
    13) UNDO deletion.
    14) REDO deletion.
    15) Look for errors.

    :return: None
    """

    import azlmbr.bus as bus
    import azlmbr.editor as editor
    import azlmbr.legacy.general as general
    import azlmbr.math as math

    from editor_python_test_tools.editor_entity_utils import EditorEntity
    from editor_python_test_tools.utils import Report, Tracer, TestHelper as helper

    with Tracer() as error_tracer:
        # Test setup begins.
        # Setup: Wait for Editor idle loop before executing Python hydra scripts then open "Base" level.
        helper.init_idle()
        helper.open_level("Physics", "Base")

        # Test steps begin.
        # 1. Create a DepthOfField entity with no components.
        depth_of_field_name = "DepthOfField"
        depth_of_field_entity = EditorEntity.create_editor_entity_at(
            math.Vector3(512.0, 512.0, 34.0), f"{depth_of_field_name}")
        Report.critical_result(Tests.depth_of_field_creation, depth_of_field_entity.exists())

        # 2. Add a DepthOfField component to DepthOfField entity.
        depth_of_field_component = depth_of_field_entity.add_component(depth_of_field_name)
        Report.critical_result(Tests.depth_of_field_component, depth_of_field_entity.has_component(depth_of_field_name))

        # 3. UNDO the entity creation and component addition.
        # Requires 3 UNDO calls to remove the Entity completely.
        for x in range(3):
            general.undo()
        general.idle_wait_frames(1)
        Report.result(Tests.creation_undo, not depth_of_field_entity.exists())

        # 4. REDO the entity creation and component addition.
        # Requires 3 REDO calls to match the previous 3 UNDO calls.
        for x in range(3):
            general.redo()
        general.idle_wait_frames(1)
        Report.result(Tests.creation_redo, depth_of_field_entity.exists())

        # 5. Add Post FX Layer component since it is required by the DepthOfField component.
        post_fx_layer = "PostFX Layer"
        depth_of_field_entity.add_component(post_fx_layer)
        Report.result(Tests.post_fx_component, depth_of_field_entity.has_component(post_fx_layer))

        # 6. Enter/Exit game mode.
        helper.enter_game_mode(Tests.enter_game_mode)
        general.idle_wait_frames(1)
        helper.exit_game_mode(Tests.exit_game_mode)

        # 7. Test IsHidden.
        depth_of_field_entity.set_visibility_state(False)
        is_hidden = editor.EditorEntityInfoRequestBus(bus.Event, 'IsHidden', depth_of_field_entity.id)
        Report.result(Tests.is_hidden, is_hidden is True)

        # 8. Test IsVisible.
        depth_of_field_entity.set_visibility_state(True)
        is_visible = editor.EditorEntityInfoRequestBus(bus.Event, 'IsVisible', depth_of_field_entity.id)
        Report.result(Tests.is_visible, is_visible is True)

        # 9. Add Camera entity.
        camera_name = "Camera"
        camera_entity = EditorEntity.create_editor_entity_at(math.Vector3(512.0, 512.0, 34.0), camera_name)
        Report.result(Tests.camera_creation, camera_entity.exists())

        # 10. Add Camera component to Camera entity.
        camera_entity.add_component(camera_name)
        Report.result(Tests.camera_component_added, camera_entity.has_component(camera_name))

        # 11. Set the DepthOfField components's Camera Entity to the newly created Camera entity.
        depth_of_field_camera_property_path = "Controller|Configuration|Camera Entity"
        depth_of_field_component.set_component_property_value(depth_of_field_camera_property_path, camera_entity.id)
        camera_entity_set = depth_of_field_component.get_component_property_value(depth_of_field_camera_property_path)
        Report.result(Tests.camera_property_set, camera_entity.id == camera_entity_set)

        # 12. Delete DepthOfField entity.
        depth_of_field_entity.delete()
        Report.result(Tests.entity_deleted, not depth_of_field_entity.exists())

        # 13. UNDO deletion.
        general.undo()
        Report.result(Tests.deletion_undo, depth_of_field_entity.exists())

        # 14. REDO deletion.
        general.redo()
        Report.result(Tests.deletion_redo, not depth_of_field_entity.exists())

        # 15. Look for errors.
        helper.wait_for_condition(lambda: error_tracer.has_errors, 1.0)
        Report.result(Tests.no_error_occurred, not error_tracer.has_errors)


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(AtomEditorComponents_DepthOfField_AddedToEntity)
