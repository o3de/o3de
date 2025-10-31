"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

def CreatePrefab_UnderChildEntityOfAnotherPrefab():
    """
    Test description:
    - Creates parent/child entities with the entities having a physx collider
    - Creates a prefab "Outer_prefab" and an instance based of the entities
    - Creates a prefab "Inner_prefab" inside "Outer_prefab" based the child entity contained inside of it

    Checks that the parent/child entity is correctly handled by the prefab system checking the name and that it contains the physx collider
    """

    from pathlib import Path

    from editor_python_test_tools.editor_entity_utils import EditorEntity
    from editor_python_test_tools.prefab_utils import Prefab
    from consts.physics import PHYSX_PRIMITIVE_COLLIDER as PHYSX_PRIMITIVE_COLLIDER_NAME
    import Prefab.tests.PrefabTestUtils as prefab_test_utils

    OUTER_PREFAB_NAME = 'Outer_prefab'
    INNER_PREFAB_NAME = 'Inner_prefab'
    OUTER_PREFAB_FILE_NAME = Path(__file__).stem + OUTER_PREFAB_NAME
    INNER_PREFAB_FILE_NAME = Path(__file__).stem + INNER_PREFAB_NAME
    PARENT_ENTITY_NAME = 'ParentEntity'
    CHILD_ENTITY_NAME = 'ChildEntity'

    prefab_test_utils.open_base_tests_level()

    # Creates new parent/child Entities at the root level
    # Asserts if creation didn't succeed
    parent_entity = EditorEntity.create_editor_entity_at((100.0, 100.0, 100.0), name=PARENT_ENTITY_NAME)
    assert parent_entity.id.IsValid(), "Couldn't create parent entity"
    parent_entity.add_component(PHYSX_PRIMITIVE_COLLIDER_NAME)
    assert parent_entity.has_component(PHYSX_PRIMITIVE_COLLIDER_NAME), f"Failed to add a {PHYSX_PRIMITIVE_COLLIDER_NAME}"
    child_entity = EditorEntity.create_editor_entity(parent_id=parent_entity.id, name=CHILD_ENTITY_NAME)
    assert child_entity.id.IsValid(), "Couldn't create child entity"
    child_entity.add_component(PHYSX_PRIMITIVE_COLLIDER_NAME)
    assert child_entity.has_component(PHYSX_PRIMITIVE_COLLIDER_NAME), f"Failed to add a {PHYSX_PRIMITIVE_COLLIDER_NAME}"

    # Create a prefab based on that entity and focus it
    _, outer_instance = Prefab.create_prefab([parent_entity], OUTER_PREFAB_FILE_NAME)
    outer_instance.container_entity.focus_on_owning_prefab()
    # The entities should be now inside the outer prefab instance.
    parent_entity_on_outer_instance = outer_instance.get_direct_child_entities()[0]
    child_entity_on_outer_instance = parent_entity_on_outer_instance.get_children()[0]

    # We track if the parent entity the same one by checking the name and if it still contains the component that we created before
    assert parent_entity_on_outer_instance.get_name() == PARENT_ENTITY_NAME, \
        f"Entity name inside '{OUTER_PREFAB_NAME}' doesn't match the original name, original:'{PARENT_ENTITY_NAME}' " \
        f"current:'{parent_entity_on_outer_instance.get_name()}'"
    assert parent_entity_on_outer_instance.has_component(PHYSX_PRIMITIVE_COLLIDER_NAME), \
        "Entity inside outer_prefab doesn't have the collider component it should"

    # Now, create another prefab, based on the child entity that is inside outer_prefab
    _, inner_instance = Prefab.create_prefab([child_entity_on_outer_instance], INNER_PREFAB_FILE_NAME)

    # Test undo/redo on prefab creation
    prefab_test_utils.validate_undo_redo_on_prefab_creation(inner_instance, parent_entity_on_outer_instance.id)

    # The child entity should now be inside the inner prefab instance
    child_entity_on_inner_instance = inner_instance.get_direct_child_entities()[0]
    # We track if the child entity is the same entity by checking the name and if it still contains the component that we created before
    assert child_entity_on_inner_instance.get_name() == CHILD_ENTITY_NAME, \
        f"Entity name inside '{INNER_PREFAB_NAME}' doesn't match the original name, original:'{CHILD_ENTITY_NAME}' " \
        f"current:'{child_entity_on_inner_instance.get_name()}'"
    assert child_entity_on_inner_instance.has_component(PHYSX_PRIMITIVE_COLLIDER_NAME), \
        "Entity inside inner_prefab doesn't have the collider component it should"

    # Verify hierarchy of entities:
    # Outer_prefab
    # |- ParentEntity
    # |  |- Inner_prefab 
    # |  |  |- ChildEntity
    assert child_entity_on_inner_instance.get_parent_id() == inner_instance.container_entity.id
    assert inner_instance.container_entity.get_parent_id() == parent_entity_on_outer_instance.id
    assert parent_entity_on_outer_instance.get_parent_id() == outer_instance.container_entity.id


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(CreatePrefab_UnderChildEntityOfAnotherPrefab)
