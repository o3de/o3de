"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


class Tests:
    no_error_reported_with_multiple_cameras_in_level = (
        "No error printed when moving to Game Mode with multiple cameras while in 'Be this camera'"
        "Error was printed when moving to Game Mode with multiple cameras while in 'Be this camera'"
    )


def EditorWorkflow_EditorCameraGameModeTransitionWithMultipleCamerasReportsNoErrors():
    """
    Summary:
    Ensure no warning is printed when moving to Game Mode with multiple cameras while in 'Be this camera'

    Expected Behavior:
    No warning message is printed
    """

    import azlmbr.bus as bus
    import azlmbr.camera as camera
    import azlmbr.components as components
    import azlmbr.legacy.general as general
    import azlmbr.math as math

    from editor_python_test_tools.editor_entity_utils import EditorEntity
    from editor_python_test_tools.editor_test_helper import EditorTestHelper
    from editor_python_test_tools.utils import Tracer, TestHelper

    with Tracer() as error_tracer:
        helper = EditorTestHelper(log_prefix="Editor Camera")

        helper.open_level("Base")

        # create a new entity with a camera component
        camera_entity_1 = EditorEntity.create_editor_entity(name="CameraEntity1")
        camera_entity_1.add_component("Camera")

        camera_entity_2 = EditorEntity.create_editor_entity(name="CameraEntity2")
        camera_entity_2.add_component("Camera")

        # trigger 'Be this camera'
        camera.EditorCameraViewRequestBus(
            bus.Event, "ToggleCameraAsActiveView", camera_entity_1.id)

        general.idle_wait_frames(1)

        # enter/exit game mode
        general.enter_game_mode()
        helper.wait_for_condition(lambda: general.is_in_game_mode(), 5.0)

        # check log for no error message
        camera_warned = False
        camera_warning = "Tried to change the editor camera during play game in editor; this is currently unsupported"
        TestHelper.wait_for_condition(lambda: error_tracer.has_errors or error_tracer.has_asserts, 5.0)
        for warning_info in error_tracer.warnings:
            Report.info(f"Assert: {warning_info.filename} {warning_info.function} | {warning_info.message}")
            if warning_info.message == camera_warning:
                camera_warned = True

        # wait for next frame
        general.idle_wait_frames(1)

        general.exit_game_mode()
        helper.wait_for_condition(lambda: not general.is_in_game_mode(), 5.0)

        # verify the position has been updated
        Report.result(Tests.no_error_reported_with_multiple_cameras_in_level, not camera_warned)


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(
        EditorWorkflow_EditorCameraGameModeTransitionWithMultipleCamerasReportsNoErrors)
