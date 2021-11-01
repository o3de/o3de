"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

def PrefabComplexWorflow_CreatePrefabOfChildEntity():
    """
    Test description:
    - Creates two entities, parent and child. Child entity has Parent entity as its parent.
    - Creates a prefab of the child entity.
    Test is successful if the new instanced prefab of the child has the parent entity id
    """

    CAR_PREFAB_FILE_NAME = 'car_prefab'

    from editor_python_test_tools.editor_entity_utils import EditorEntity
    from editor_python_test_tools.prefab_utils import Prefab

    import PrefabTestUtils as prefab_test_utils

    prefab_test_utils.open_base_tests_level()

    # Creates a new Entity at the root level
    # Asserts if creation didn't succeed
    parent_entity = EditorEntity.create_editor_entity_at((100.0, 100.0, 100.0))
    assert parent_entity.id.IsValid(), "Couldn't create parent entity"

    child_entity = EditorEntity.create_editor_entity(parent_id=parent_entity.id)
    assert child_entity.id.IsValid(), "Couldn't create child entity"
    assert child_entity.get_world_translation().IsClose(parent_entity.get_world_translation()), f"Child entity position{child_entity.get_world_translation().ToString()}" \
                                                                                                f" is not located at the same position as the parent{parent_entity.get_world_translation().ToString()}"

    # Asserts if prefab creation doesn't succeed
    child_prefab, child_instance = Prefab.create_prefab([child_entity], CAR_PREFAB_FILE_NAME)
    child_entity_on_child_instance = child_instance.get_direct_child_entities()[0]
    assert child_instance.container_entity.get_parent_id().IsValid(), "Newly instanced entity has no parent"
    assert child_instance.container_entity.get_parent_id() == parent_entity.id, "Newly instanced entity parent does not match the expected parent"
    assert child_instance.container_entity.get_world_translation().IsClose(parent_entity.get_world_translation()), "Newly instanced entity position is not located at the same position as the parent"
    # Move the parent position, it should update the child position
    parent_entity.set_world_translation((200.0, 200.0, 200.0))
    child_instance_translation = child_instance.container_entity.get_world_translation()
    assert child_instance_translation.IsClose(azlmbr.math.Vector3(200.0, 200.0, 200.0)), f"Instance position position{child_instance_translation.ToString()} didn't get updated" \
                                                                                         f" to the same position as the parent{parent_entity.get_world_translation().ToString()}"
    child_translation = child_entity_on_child_instance.get_world_translation()
    assert child_translation.IsClose(azlmbr.math.Vector3(200.0, 200.0, 200.0)), f"Entity position{child_translation.ToString()} of the instance didn't get updated" \
                                                                                f" to the same position as the parent{parent_entity.get_world_translation().ToString()}"

if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(PrefabComplexWorflow_CreatePrefabOfChildEntity)
