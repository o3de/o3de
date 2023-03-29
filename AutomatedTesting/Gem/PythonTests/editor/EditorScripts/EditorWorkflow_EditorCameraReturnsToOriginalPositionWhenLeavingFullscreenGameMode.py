"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


class Tests:
    camera_moved_to_position_edit_mode = (
        "Editor camera view moved to the new position in edit mode"
        "Editor camera view did not move to the new position in edit mode"
    )

    camera_returned_to_initial_position_in_edit_mode_after_fullscreen_preview = (
        "Editor camera view restored after returning from game mode in fullscreen"
        "Editor camera view was not restored after returning from game mode in fullscreen"
    )


def EditorWorkflow_EditorCameraReturnsToOriginalPositionWhenLeavingFullscreenGameMode():
    """
    Summary:
    Test camera position when entering/leaving game mode fullscreen preview

    Expected Behavior:
    Editor camera view returns to original editor camera position after leaving game mode fullscreen preview
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

    def set_current_view_position_with_vector3(view_position: math.Vector3):
        general.set_current_view_position(
            view_position.x, view_position.y, view_position.z)

    helper = EditorTestHelper(log_prefix="Editor Camera")

    helper.open_level("Base")

    # where we expect the editor camera to end up
    editor_camera_position = math.Vector3(20.0, 40.0, 60.0)
    set_current_view_position_with_vector3(editor_camera_position)

    general.idle_wait_frames(1)

    initial_camera_position = get_current_view_position_as_vector3()
    initial_camera_position_position_set = initial_camera_position.IsClose(editor_camera_position)
    Report.result(Tests.camera_moved_to_position_edit_mode, initial_camera_position_position_set)

    # enter/exit game mode
    general.enter_game_mode_fullscreen()
    helper.wait_for_condition(lambda: general.is_in_game_mode(), 5.0)

    general.exit_game_mode()
    helper.wait_for_condition(lambda: not general.is_in_game_mode(), 5.0)

    # ensure camera position has returned to the initial camera position
    actual_camera_position = get_current_view_position_as_vector3()
    position_restored = actual_camera_position.IsClose(initial_camera_position)
    # verify the position has been updated
    Report.result(Tests.camera_returned_to_initial_position_in_edit_mode_after_fullscreen_preview, position_restored)


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(
        EditorWorkflow_EditorCameraReturnsToOriginalPositionWhenLeavingFullscreenGameMode)
