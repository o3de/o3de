"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


class Tests:
    lc_component_added = (
        "Landscape Canvas component successfully added to entity",
        "Failed to add Landscape Canvas component to entity"
    )
    lc_component_removed = (
        "Landscape Canvas component successfully removed from entity",
        "Failed to remove Landscape Canvas component from entity"
    )


def Component_AddedRemoved():
    """
    Summary:
    This test verifies that the Landscape Canvas component can be added to/removed from an entity.

    Expected Behavior:
    Closing a tabbed graph only closes the appropriate graph.

    Test Steps:
     1) Open a simple level
     2) Create a new entity
     3) Add a Landscape Canvas component to the entity
     4) Remove the Landscape Canvas component from the entity

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

    # Create an Entity at the root of the level
    newEntityId = editor.ToolsApplicationRequestBus(bus.Broadcast, 'CreateNewEntity', entity.EntityId())

    # Find the component TypeId for our Landscape Canvas component
    landscape_canvas_type_id = hydra.get_component_type_id("Landscape Canvas")

    # Add the Landscape Canvas Component to our Entity
    componentOutcome = editor.EditorComponentAPIBus(bus.Broadcast, 'AddComponentsOfType', newEntityId, [landscape_canvas_type_id])
    components = componentOutcome.GetValue()
    landscapeCanvasComponent = components[0]

    # Validate the Landscape Canvas Component exists
    hasComponent = editor.EditorComponentAPIBus(bus.Broadcast, 'HasComponentOfType', newEntityId, landscape_canvas_type_id)
    Report.result(Tests.lc_component_added, hasComponent)

    # Remove the Landscape Canvas Component
    editor.EditorComponentAPIBus(bus.Broadcast, 'RemoveComponents', [landscapeCanvasComponent])

    # Validate the Landscape Canvas Component is no longer on our Entity
    hasComponent = editor.EditorComponentAPIBus(bus.Broadcast, 'HasComponentOfType', newEntityId, landscape_canvas_type_id)
    Report.result(Tests.lc_component_removed, not hasComponent)


if __name__ == "__main__":

    from editor_python_test_tools.utils import Report
    Report.start_test(Component_AddedRemoved)