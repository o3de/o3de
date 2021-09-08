"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


def GradientTransform_ComponentIncompatibleWithSpawners():
    """
    Summary:
    A simple level is opened. A New entity is created with components Gradient Transform Modifier and Box Shape.
    Adding a component Vegetation Layer Spawner to the same entity.

    Expected Behavior:
    The Vegetation Layer Spawner is deactivated and it is communicated that it is incompatible with Gradient
    Transform Modifier

    Test Steps:
     1) Open a simple level
     2) Create a new entity with components Gradient Transform Modifier and Box Shape
     3) Make sure all components are enabled in Entity
     4) Add Vegetation Layer Spawner to the same entity
     5) Make sure newly added component is disabled

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

    # 4) Add Vegetation Layer Spawner to the same entity
    gradient.components.append(hydra.add_component("Vegetation Layer Spawner", gradient_id))

    # 5) Make sure newly added component is disabled
    spawner_component_disabled = (
        "Spawner component is disabled",
        "Spawner component is unexpectedly enabled"
    )
    Report.result(spawner_component_disabled, not is_enabled(gradient.components[2]))


if __name__ == "__main__":

    from editor_python_test_tools.utils import Report
    Report.start_test(GradientTransform_ComponentIncompatibleWithSpawners)
