"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


def GradientTransform_ComponentIncompatibleWithExpectedGradients():
    """
    Summary:
    A New level is created. A New entity is created with components Gradient Transform Modifier and Box Shape.
    Adding components Constant Gradient, Altitude Gradient, Gradient Mixer, Reference Gradient, Shape
    Falloff Gradient, Slope Gradient and Surface Mask Gradient to the same entity.

    Expected Behavior:
    All added components are disabled and inform the user that they are incompatible with the Gradient Transform
    Modifier

    Test Steps:
     1) Create level
     2) Create a new entity with components Gradient Transform Modifier and Box Shape
     3) Make sure all components are enabled in Entity
     4) Add Constant Gradient, Altitude Gradient, Gradient Mixer, Reference Gradient, Shape
        Falloff Gradient, Slope Gradient and Surface Mask Gradient to the same entity
     5) Make sure all newly added components are disabled

    Note:
    - This test file must be called from the Open 3D Engine Editor command terminal
    - Any passed and failed tests are written to the Editor.log file.
            Parsing the file or running a log_monitor are required to observe the test results.

    :return: None
    """

    import azlmbr.bus as bus
    import azlmbr.editor as editor
    import azlmbr.math as math
    import azlmbr.entity as EntityId

    import editor_python_test_tools.hydra_editor_utils as hydra
    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper as helper

    def is_enabled(EntityComponentIdPair):
        return editor.EditorComponentAPIBus(bus.Broadcast, "IsComponentEnabled", EntityComponentIdPair)

    # 1) Open an existing simple level
    helper.init_idle()
    helper.open_level("Physics", "Base")

    # 2) Create a new entity with components Gradient Transform Modifier and Box Shape
    entity_position = math.Vector3(125.0, 136.0, 32.0)
    components_to_add = ["Gradient Transform Modifier", "Box Shape"]
    gradient_id = editor.ToolsApplicationRequestBus(
        bus.Broadcast, "CreateNewEntityAtPosition", entity_position, EntityId.EntityId()
    )
    gradient = hydra.Entity("gradient", gradient_id)

    gradient.components = []

    for component in components_to_add:
        gradient.components.append(hydra.add_component(component, gradient_id))
    entity_created = (
        "Entity created successfully",
        "Failed to create entity"
    )
    Report.critical_result(entity_created, gradient_id.isValid())

    # 3) Make sure all components are enabled in Entity
    index = 0
    for component in components_to_add:
        components_enabled = (
            f"{component} is enabled",
            f"{component} is unexpectedly disabled"
        )
        Report.critical_result(components_enabled, is_enabled(gradient.components[index]))
        index += 1

    # 4) Add Constant Gradient, Altitude Gradient, Gradient Mixer, Reference Gradient, Shape
    #    Falloff Gradient, Slope Gradient and Surface Mask Gradient to the same entity
    new_components_to_add = [
        "Constant Gradient",
        "Altitude Gradient",
        "Gradient Mixer",
        "Reference Gradient",
        "Shape Falloff Gradient",
        "Slope Gradient",
        "Surface Mask Gradient",
    ]
    new_components_enabled = False
    for component in new_components_to_add:
        gradient.components.append(hydra.add_component(component, gradient_id))
        gradient_components_disabled = (
            f"{component} is disabled",
            f"{component} is enabled, but should be disabled"
        )
        Report.result(gradient_components_disabled, not is_enabled(gradient.components[2]))
        if not is_enabled(gradient.components[2]):
            editor.EditorComponentAPIBus(bus.Broadcast, "RemoveComponents", component)


if __name__ == "__main__":

    from editor_python_test_tools.utils import Report
    Report.start_test(GradientTransform_ComponentIncompatibleWithExpectedGradients)
