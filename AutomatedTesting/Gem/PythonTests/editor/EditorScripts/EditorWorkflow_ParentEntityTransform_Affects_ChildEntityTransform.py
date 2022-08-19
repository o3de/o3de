"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

class Tests:
    create_entity_statuses = (
        "Parent entity created successfully",
        "Failed to create a parent entity"
    )
    create_child_entity_statuses = (
        "Child entity created successfully",
        "Failed to create a child entity"
    )
    move_parent_entity_statuses = (
        "Parent entity moved successfully",
        "Failed to move the parent entity"
    )
    move_child_entity_statuses = (
        "Child entity moved successfully",
        "Failed to move the child entity"
    )
    parent_entity_after_undo_statuses = (
        "Parent entity moved back to original position after undo successfully",
        "Failed to move the parent entity back to original position after undo"
    )
    child_entity_after_undo_statuses = (
        "Child entity moved back to original position after undo successfully",
        "Failed to move the child entity back to original position after undo"
    )
    parent_entity_after_redo_statuses = (
        "Parent entity moved to desired position after redo successfully",
        "Failed to move the parent entity to desired position after redo"
    )
    child_entity_after_redo_statuses = (
        "Parent entity moved to desired position after redo successfully",
        "Failed to move the parent entity to desired position after redo"
    )

def EditorWorkflow_ParentEntityTransform_Affects_ChildEntityTransform():
    """
        Performing basic test in editor
            01. Load base level
            02. create parent entity and set name
            03. create child entity and set a name
            04. Update transform position of parent entity and verify that child got the same transform
            05. Undo and verify both parent and child went back to original transform
            06. Redo and verify that both parent and child moved again to new transform
    """

    import azlmbr.bus as bus
    import azlmbr.editor as editor
    import azlmbr.legacy.general as general
    import azlmbr.globals as globals
    import editor_python_test_tools.hydra_editor_utils as hydra

    from editor_python_test_tools.editor_entity_utils import EditorEntity, EditorComponent
    from editor_python_test_tools.wait_utils import PrefabWaiter

    expected_translate_position = azlmbr.math.Vector3(10.0, 0.0, 0.0)
    default_translate_position = azlmbr.math.Vector3(0.0, 0.0, 0.0)

    # 01. load an existing level
    hydra.open_base_level()

    # 02. create parent entity and set name
    parent_entity = EditorEntity.create_editor_entity_at((0.0, 0.0, 0.0), name="Parent_Entity")
    Report.result(Tests.create_entity_statuses, parent_entity.exists())

    # 03. Create child Entity to above created parent entity and set a name
    child_entity = EditorEntity.create_editor_entity("Child_Entity", parent_entity.id)
    Report.result(Tests.create_child_entity_statuses, child_entity.exists())
    PrefabWaiter.wait_for_propagation()

    # 04. Move the parent entity to a new postion.
    get_transform_component_outcome = editor.EditorComponentAPIBus(
        bus.Broadcast, "GetComponentOfType", parent_entity.id, globals.property.EditorTransformComponentTypeId
    )
    parent_transform_component = EditorComponent(globals.property.EditorTransformComponentTypeId)
    parent_transform_component.id = get_transform_component_outcome.GetValue()
    hydra.set_component_property_value(parent_transform_component.id, "Values|Translate", expected_translate_position)
    PrefabWaiter.wait_for_propagation()
    Report.result(Tests.move_parent_entity_statuses,
                  parent_entity.validate_world_translate_position(expected_translate_position))
    Report.result(Tests.move_child_entity_statuses,
                  child_entity.validate_world_translate_position(expected_translate_position))

    # 05. Undo the translation of parent entity.
    general.undo()
    PrefabWaiter.wait_for_propagation()
    Report.result(Tests.move_parent_entity_statuses,
                  parent_entity.validate_world_translate_position(default_translate_position))
    Report.result(Tests.move_child_entity_statuses,
                  child_entity.validate_world_translate_position(default_translate_position))

    # 06. Redo the translation of parent entity.
    general.redo()
    PrefabWaiter.wait_for_propagation()
    Report.result(Tests.move_parent_entity_statuses,
                  parent_entity.validate_world_translate_position(expected_translate_position))
    Report.result(Tests.move_child_entity_statuses,
                  child_entity.validate_world_translate_position(expected_translate_position))


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report

    Report.start_test(EditorWorkflow_ParentEntityTransform_Affects_ChildEntityTransform)
