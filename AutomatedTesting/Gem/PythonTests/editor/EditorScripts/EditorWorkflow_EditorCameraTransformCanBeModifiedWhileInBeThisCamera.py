"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


class Tests:
    camera_position_updated_while_in_be_this_came = (
        "Editor camera view was changed externally while in 'Be this camera'"
        "Editor camera view was not changed externally while in 'Be this camera'"
    )


def EditorWorkflow_EditorCameraTransformCanBeModifiedWhileInBeThisCamera():
    """
    Summary:
    Test camera position when entering/leaving game mode

    Expected Behavior:
    Editor camera view returns to original editor camera position after leaving game mode
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
    entity_camera_position = math.Vector3(20.0, 40.0, 60.0)

    # create a new entity with a camera component
    camera_entity = EditorEntity.create_editor_entity(name="CameraEntity")
    camera_entity.add_component("Camera")

    # position the entity in the world
    components.TransformBus(bus.Event, "SetWorldTranslation",
                            camera_entity.id, entity_camera_position)

    # trigger 'Be this camera'
    camera.EditorCameraViewRequestBus(
        bus.Event, "ToggleCameraAsActiveView", camera_entity.id)

    # where we expect the camera to move to
    next_entity_camera_position = math.Vector3(80.0, 160.0, 240.0)

    components.TransformBus(bus.Event, "SetWorldTranslation",
                            camera_entity.id, next_entity_camera_position)

    # wait for next frame
    general.idle_wait_frames(1)

    # ensure camera position has returned to the initial camera position
    actual_camera_position = get_current_view_position_as_vector3()

    position_updated = actual_camera_position.IsClose(next_entity_camera_position)

    # verify the position has been updated
    Report.result(Tests.camera_position_updated_while_in_be_this_came, position_updated)


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(
        EditorWorkflow_EditorCameraTransformCanBeModifiedWhileInBeThisCamera)
