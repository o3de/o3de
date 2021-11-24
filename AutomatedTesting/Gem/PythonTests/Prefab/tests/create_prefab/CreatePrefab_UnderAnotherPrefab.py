"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

def CreatePrefab_UnderAnotherPrefab():
    """
    Test description:
    - Creates an entity with a physx collider
    - Creates a prefab "Outer_prefab" and an instance based of that entity 
    - Creates a prefab "Inner_prefab" inside "Outer_prefab" based the entity contained inside of it
    Checks that the entity is correctly handlded by the prefab system checking the name and that it contains the physx collider
    """

    from editor_python_test_tools.editor_entity_utils import EditorEntity
    from editor_python_test_tools.prefab_utils import Prefab

    import Prefab.tests.PrefabTestUtils as prefab_test_utils

    prefab_test_utils.open_base_tests_level()

    # Creates a new Entity at the root level
    # Asserts if creation didn't succeed
    entity = EditorEntity.create_editor_entity_at((100.0, 100.0, 100.0), name = "TestEntity")
    assert entity.id.IsValid(), "Couldn't create entity"
    entity.add_component("PhysX Collider")
    assert entity.has_component("PhysX Collider"), "Attempted to add a PhysX Collider but no physx collider collider was found afterwards"

    # Create a prefab based on that entity
    outer_prefab, outer_instance = Prefab.create_prefab([entity], "Outer_prefab")
    # The test should be now inside the outer prefab instance.
    entity = outer_instance.get_direct_child_entities()[0]
    # We track if that is the same entity by checking the name and if it still contains the component that we created before
    assert entity.get_name() == "TestEntity", f"Entity name inside outer_prefab doesn't match the original name, original:'TestEntity' current:'{entity.get_name()}'"
    assert entity.has_component("PhysX Collider"), "Entity name inside outer_prefab doesn't have the collider component it should"

    # Now, create another prefab, based on the entity that is inside outer_prefab
    inner_prefab, inner_instance = Prefab.create_prefab([entity], "Inner_prefab")
    # The test entity should now be inside the inner prefab instance
    entity = inner_instance.get_direct_child_entities()[0]
    # We track if that is the same entity by checking the name and if it still contains the component that we created before
    assert entity.get_name() == "TestEntity", f"Entity name inside inner_prefab doesn't match the original name, original:'TestEntity' current:'{entity.get_name()}'"
    assert entity.has_component("PhysX Collider"), "Entity name inside inner_prefab doesn't have the collider component it should"

    # Verify hierarchy of entities:
    # Outer_prefab
    # |- Inner_prefab 
    # |  |- TestEntity
    assert entity.get_parent_id() == inner_instance.container_entity.id
    assert inner_instance.container_entity.get_parent_id() == outer_instance.container_entity.id


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(CreatePrefab_UnderAnotherPrefab)
