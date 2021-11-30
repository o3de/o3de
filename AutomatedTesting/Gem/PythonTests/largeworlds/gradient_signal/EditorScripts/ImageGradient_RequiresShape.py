"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


class Tests:
    disabled_without_shape = (
        "Image Gradient component is disabled without a Shape component on the Entity",
        "Image Gradient component is unexpectedly enabled without a Shape component on the Entity",
    )
    enabled_with_shape = (
        "Image Gradient component is enabled now that the Entity has a Shape",
        "Image Gradient component is still disabled alongside a Shape component",
    )


def ImageGradient_RequiresShape():
    """
    Summary:
    This test verifies that the Image Gradient component is dependent on a shape component.

    Expected Result:
    Gradient Transform Modifier component is disabled until a shape component is added to the entity.

    Test Steps:
     1) Open level
     2) Create a new entity with a Image Gradient component
     3) Verify the component is disabled until a shape component is also added to the entity

    Note:
    - This test file must be called from the Open 3D Engine Editor command terminal
    - Any passed and failed tests are written to the Editor.log file.
            Parsing the file or running a log_monitor are required to observe the test results.

    :return: None
    """

    import azlmbr.bus as bus
    import azlmbr.editor as editor
    import azlmbr.entity as entity

    import editor_python_test_tools.hydra_editor_utils as hydra
    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper as helper

    # Open an existing simple level
    helper.init_idle()
    helper.open_level("Physics", "Base")

    # Create a new Entity in the level
    entity_id = editor.ToolsApplicationRequestBus(bus.Broadcast, 'CreateNewEntity', entity.EntityId())

    # Add an Image Gradient and Gradient Transform Component (should be disabled until a Shape exists on the Entity)
    image_gradient_component = hydra.add_component('Image Gradient', entity_id)
    hydra.add_component('Gradient Transform Modifier', entity_id)

    # Verify the Image Gradient Component is not active before adding the Shape
    active = editor.EditorComponentAPIBus(bus.Broadcast, 'IsComponentEnabled', image_gradient_component)
    Report.result(Tests.disabled_without_shape, not active)

    # Add a Shape component to the same Entity
    hydra.add_component('Box Shape', entity_id)

    # Check if the Image Gradient Component is active now after adding the Shape
    active = editor.EditorComponentAPIBus(bus.Broadcast, 'IsComponentEnabled', image_gradient_component)
    Report.result(Tests.enabled_with_shape, active)


if __name__ == "__main__":

    from editor_python_test_tools.utils import Report
    Report.start_test(ImageGradient_RequiresShape)
