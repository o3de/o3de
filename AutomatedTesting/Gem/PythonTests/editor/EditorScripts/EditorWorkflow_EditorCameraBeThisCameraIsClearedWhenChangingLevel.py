"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


class Tests:
    first_level_created = (
        "Successfully created first level",
        "Failed to create first level"
    )
    second_level_created = (
        "Successfully created second level",
        "Failed to create second level"
    )
    editor_camera_initial_position_identical_in_first_and_second_level = (
        "Initial camera positions match in first and second level",
        "Initial camera positions do not match in first and second level",
    )
    entity_position_changed_when_not_in_be_this_camera = (
        "Entity position remained the same when setting view position while not in 'Be this camera'",
        "Entity position changed when setting view position while not in 'Be this camera'"
    )
    view_position_changed_when_moving_entity_when_not_in_be_this_camera = (
        "View position remained the same when moving entity while not in 'Be this camera' mode",
        "View position changed when moving entity while not in 'Be this camera' mode",
    )
    internal_camera_error = (
        "No internal camera error reported"
        "Internal camera error reported"
    )
    view_bookmark_error = (
        "No view bookmark error reported"
        "View bookmark error reported"
    )


def EditorWorkflow_EditorCameraBeThisCameraIsClearedWhenChangingLevel():
    """
    Summary:
    Ensures the editor does not stay in 'Be this camera' when changing level while still in
    'Be this camera' mode

    Expected Behavior:
    Moving the camera does not change the transform of the entity with a camera component on it
    """

    import azlmbr.bus as bus
    import azlmbr.camera as camera
    import azlmbr.components as components
    import azlmbr.legacy.general as general
    import azlmbr.math as math

    from editor_python_test_tools.editor_entity_utils import EditorEntity
    from editor_python_test_tools.editor_test_helper import EditorTestHelper
    from editor_python_test_tools.utils import Tracer, TestHelper

    def get_current_view_position_as_vector3() -> math.Vector3:
        view_position = general.get_current_view_position()
        x = view_position.get_property('x')
        y = view_position.get_property('y')
        z = view_position.get_property('z')
        return math.Vector3(float(x), float(y), float(z))

    def set_current_view_position_with_vector3(view_position: math.Vector3):
        general.set_current_view_position(
            view_position.x, view_position.y, view_position.z)

    with Tracer() as error_tracer:
        helper = EditorTestHelper(log_prefix="Editor Camera")

        template_name = "Prefabs/Default_Level.prefab"

        # note: level name needs to match name in parent TestSuite_Main file
        # see TestAutomation(EditorTestSuite)
        first_level_created = helper.create_level(template_name, "tmp_level_1")
        Report.critical_result(Tests.first_level_created, first_level_created)

        # record initial viewport position (new level default)
        starting_view_position_level_1 = get_current_view_position_as_vector3()

        # default camera that comes with a new level
        first_existing_camera_entity = EditorEntity.find_editor_entity(
            "Camera")

        # trigger 'Be this camera'
        camera.EditorCameraViewRequestBus(
            bus.Event, "ToggleCameraAsActiveView", first_existing_camera_entity.id)

        # wait for next frame
        general.idle_wait_frames(1)

        # note: level name needs to match name in parent TestSuite_Main file
        # see TestAutomation(EditorTestSuite)
        second_level_created = helper.create_level(template_name, "tmp_level_2")
        Report.critical_result(
            Tests.second_level_created, second_level_created)

        # record initial viewport position for next level (new level default)
        starting_view_position_level_2 = get_current_view_position_as_vector3()

        # ensure initial position matches first and second level
        initial_positions_match = starting_view_position_level_1.IsClose(
            starting_view_position_level_2)
        Report.result(
            Tests.editor_camera_initial_position_identical_in_first_and_second_level, initial_positions_match)

        # default camera the comes with a new level (from newly created level)
        second_existing_camera_entity = EditorEntity.find_editor_entity(
            "Camera")

        # change the position of the editor camera
        expected_view_position = math.Vector3(123.0, 456.0, 789.0)
        set_current_view_position_with_vector3(expected_view_position)

        general.idle_wait_frames(1)

        # verify that setting the view position did not change the entity position
        entity_position = components.TransformBus(
            bus.Event, "GetWorldTranslation", second_existing_camera_entity.id)
        entity_position_changed = entity_position.IsClose(
            expected_view_position)
        Report.result(
            Tests.entity_position_changed_when_not_in_be_this_camera, not entity_position_changed)

        # set the position of the camera entity (not currently in 'Be this camera' mode)
        next_entity_position = math.Vector3(20.0, 40.0, 60.0)
        components.TransformBus(
            bus.Event, "SetWorldTranslation", second_existing_camera_entity.id, next_entity_position)

        next_view_position = get_current_view_position_as_vector3()
        view_matches_entity = next_view_position.IsClose(next_entity_position)
        Report.result(
            Tests.view_position_changed_when_moving_entity_when_not_in_be_this_camera, not view_matches_entity)

        # check log for no view or view bookmark related error messages
        view_errored = False
        view_error = "Internal logic error - active view changed to an entity which is not an editor camera. Please report this as a bug."
        bookmark_errored = False
        bookmark_error = "Couldn't find or create LocalViewBookmarkComponent."
        TestHelper.wait_for_condition(
            lambda: error_tracer.has_errors or error_tracer.has_asserts, 5.0)
        for error_info in error_tracer.errors:
            Report.info(
                f"Error: {error_info.filename} {error_info.function} | {error_info.message}")
            if error_info.message == view_error:
                view_errored = True
            if error_info.message == bookmark_error:
                bookmark_errored = True

        # verify no errors were reported
        Report.result(Tests.internal_camera_error, not view_errored)
        Report.result(Tests.view_bookmark_error, not bookmark_errored)


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(
        EditorWorkflow_EditorCameraBeThisCameraIsClearedWhenChangingLevel)
