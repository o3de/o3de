"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


class Tests:
    entity_created = (
        "Entity created successfully",
        "Failed to create entity"
    )
    components_added = (
        "All expected components added to entity",
        "Failed to add expected components to entity"
    )
    higher_zoom_value_set = (
        "Frequency Zoom is equal to expected value",
        "Frequency Zoom is not equal to expected value"
    )


def GradientTransform_FrequencyZoomCanBeSetBeyondSliderRange():
    """
    Summary:
    Frequency Zoom can manually be set higher than 8 in a random noise gradient

    Expected Behavior:
    The value properly changes, despite the value being outside of the slider limit

    Test Steps:
     1) Open level
     2) Create entity
     3) Add components to the entity
     4) Set the frequency value of the component
     5) Verify if the frequency value is set to higher value

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

    # 1) Open an existing simple level
    helper.init_idle()
    helper.open_level("Physics", "Base")

    # 2) Create entity
    entity_position = math.Vector3(125.0, 136.0, 32.0)
    entity_id = editor.ToolsApplicationRequestBus(
        bus.Broadcast, "CreateNewEntityAtPosition", entity_position, EntityId.EntityId()
    )
    Report.critical_result(Tests.entity_created, entity_id.IsValid())

    # 3) Add components to the entity
    components_to_add = ["Random Noise Gradient", "Gradient Transform Modifier", "Box Shape"]
    entity = hydra.Entity("entity", entity_id)
    entity.components = []
    for component in components_to_add:
        entity.components.append(hydra.add_component(component, entity_id))
    Report.critical_result(Tests.components_added, len(entity.components) == 3)

    # 4) Set the frequency value of the component
    hydra.get_set_test(entity, 1, "Configuration|Frequency Zoom", 10)

    # 5) Verify if the frequency value is set to higher value
    curr_value = hydra.get_component_property_value(entity.components[1], "Configuration|Frequency Zoom")
    Report.result(Tests.higher_zoom_value_set, curr_value == 10.0)


if __name__ == "__main__":

    from editor_python_test_tools.utils import Report
    Report.start_test(GradientTransform_FrequencyZoomCanBeSetBeyondSliderRange)
