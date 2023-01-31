"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


class Tests:
    camera_moved_to_entity_position_game_mode = (
        "Editor camera view moved to the camera entity position in game mode"
        "Editor camera view did not move to the camera entity position in game mode"
    )

    camera_returned_to_initial_position = (
        "Editor camera view restored after returning from game mode"
        "Editor camera view was not restored after returning from game mode"
    )


def EditorWorkflow_EditorCameraReturnsToOriginalPositionWhenLeavingGameMode():
    """
    Summary:
    Test camera position when entering/leaving game mode

    Expected Behavior:
    Editor camera view returns to original editor camera position after leaving game mode
    """

    import azlmbr.bus as bus
    import azlmbr.editor as editor
    import azlmbr.components as components
    import azlmbr.legacy.general as general
    import azlmbr.math as math

    from editor_python_test_tools.editor_entity_utils import EditorEntity
    from editor_python_test_tools.editor_test_helper import EditorTestHelper
    from editor_python_test_tools.utils import Report, TestHelper

    def get_current_view_position_as_vector3() -> math.Vector3:
        view_position = general.get_current_view_position()
        x = view_position.get_property('x')
        y = view_position.get_property('y')
        z = view_position.get_property('z')
        return math.Vector3(float(x), float(y), float(z))

    helper = EditorTestHelper(log_prefix="Editor Camera")

    helper.open_level("Base")

    # where we expect the camera to end up
    entity_camera_position = math.Vector3(20.0, 40.0, 60.0)

    # begin recording an undo batch for the newly created entity
    editor.ToolsApplicationRequestBus(bus.Broadcast, 'BeginUndoBatch', "Create camera entity")

    # create a new entity with a camera component
    camera_entity = EditorEntity.create_editor_entity(name="CameraEntity")
    camera_entity.add_component("Camera")

    # explicitly track the entity in the newly created undo batch
    editor.ToolsApplicationRequestBus(bus.Broadcast, 'AddDirtyEntity', camera_entity.id)

    # position the entity in the world
    components.TransformBus(bus.Event, "SetWorldTranslation",
                            camera_entity.id, entity_camera_position)

    # after modifying the entity, end the undo batch (this will ensure the change is tracked
    # when moving to/from game mode)
    editor.ToolsApplicationRequestBus(bus.Broadcast, 'EndUndoBatch')

    general.idle_wait_frames(1)

    # note: this won't match the entity transform, this will be the default camera position
    initial_camera_position = get_current_view_position_as_vector3()

    # enter/exit game mode
    general.enter_game_mode()
    helper.wait_for_condition(lambda: general.is_in_game_mode(), 5.0)

    game_mode_camera_position = get_current_view_position_as_vector3()

    position_changed_game_mode = game_mode_camera_position.IsClose(entity_camera_position)
    Report.result(Tests.camera_moved_to_entity_position_game_mode, position_changed_game_mode)

    general.exit_game_mode()
    helper.wait_for_condition(lambda: not general.is_in_game_mode(), 5.0)

    # ensure camera position has returned to the initial camera position
    actual_camera_position = get_current_view_position_as_vector3()

    position_restored = actual_camera_position.IsClose(initial_camera_position)

    # verify the position has been updated
    Report.result(Tests.camera_returned_to_initial_position, position_restored)


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(
        EditorWorkflow_EditorCameraReturnsToOriginalPositionWhenLeavingGameMode)
