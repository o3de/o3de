"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


class Tests:
    disabled_without_shape = (
        "Gradient Transform Modifier component is disabled without a Shape component on the Entity",
        "Gradient Transform Modifier component is unexpectedly enabled without a Shape component on the Entity",
    )
    enabled_with_shape = (
        "Gradient Transform Modifier component is enabled now that the Entity has a Shape",
        "Gradient Transform Modifier component is still disabled alongside a Shape component",
    )


def GradientTransform_RequiresShape():
    """
    Summary:
    This test verifies that the Gradient Transform Modifier component is dependent on a shape component.

    Expected Result:
    Gradient Transform Modifier component is disabled until a shape component is added to the entity.

    Test Steps:
     1) Open level
     2) Create a new entity with a Gradient Transform Modifier component
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

    # Add a Gradient Transform Component (that will be disabled since there is no shape on the Entity)
    gradient_transform_component = hydra.add_component('Gradient Transform Modifier', entity_id)

    # Verify the Gradient Transform Component is not active before adding the Shape
    active = editor.EditorComponentAPIBus(bus.Broadcast, 'IsComponentEnabled', gradient_transform_component)
    Report.result(Tests.disabled_without_shape, not active)

    # Add a Shape component to the same Entity
    hydra.add_component('Box Shape', entity_id)

    # Check if the Gradient Transform Component is active now after adding the Shape
    active = editor.EditorComponentAPIBus(bus.Broadcast, 'IsComponentEnabled', gradient_transform_component)
    Report.result(Tests.enabled_with_shape, active)


if __name__ == "__main__":

    from editor_python_test_tools.utils import Report
    Report.start_test(GradientTransform_RequiresShape)
