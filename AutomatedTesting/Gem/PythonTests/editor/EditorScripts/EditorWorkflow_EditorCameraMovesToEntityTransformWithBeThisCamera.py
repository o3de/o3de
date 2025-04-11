"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


class Tests:
    camera_moved_to_entity_transform = (
        "Editor camera view moved to match entity"
        "Editor camera view did not move to match entity"
    )


def EditorWorkflow_EditorCameraMovesToEntityTransformWithBeThisCamera():
    """
    Summary:
    Editor camera view moves to entity transform for 'Be this camera' functionality

    Expected Behavior:
    After triggering 'Be this camera', the camera moves to the entity position
    """

    import azlmbr.bus as bus
    import azlmbr.camera as camera
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
    expected_camera_position = math.Vector3(50.0, 50.0, 50.0)

    # create a new entity with a camera component
    camera_entity = EditorEntity.create_editor_entity_at(
        (0.0, 0.0, 0.0), name="CameraEntity")
    camera_entity.add_component("Camera")

    # position the entity in the world
    components.TransformBus(bus.Event, "SetWorldTranslation",
                            camera_entity.id, expected_camera_position)

    # trigger 'Be this camera'
    camera.EditorCameraViewRequestBus(
        bus.Event, "ToggleCameraAsActiveView", camera_entity.id)

    # ensure camera position has updated to the entity position
    actual_camera_position = get_current_view_position_as_vector3()
    position_updated = actual_camera_position.IsClose(expected_camera_position)

    # verify the position has been updated
    Report.result(Tests.camera_moved_to_entity_transform, position_updated)


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(
        EditorWorkflow_EditorCameraMovesToEntityTransformWithBeThisCamera)
