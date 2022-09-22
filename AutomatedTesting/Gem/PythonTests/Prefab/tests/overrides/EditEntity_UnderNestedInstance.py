"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

def EditEntity_UnderNestedInstance():

    from editor_python_test_tools.editor_entity_utils import EditorEntity, EditorComponent
    from editor_python_test_tools.prefab_utils import Prefab
    from editor_python_test_tools.wait_utils import PrefabWaiter
    from pathlib import Path
    import azlmbr.legacy.general as general
    import Prefab.tests.PrefabTestUtils as prefab_test_utils
    import azlmbr.bus as bus
    import azlmbr.editor as editor
    import azlmbr.globals as globals
    import editor_python_test_tools.hydra_editor_utils as hydra

    prefab_test_utils.open_base_tests_level()

    NESTED_PREFABS_FILE_NAME_PREFIX = Path(__file__).stem + '_' + 'nested_prefabs_'
    NESTED_PREFABS_NAME_PREFIX = 'NestedPrefabs_Prefab_'
    NESTED_PREFABS_TEST_ENTITY_NAME = 'TestEntity'
    CREATION_POSITION = azlmbr.math.Vector3(0.0, 0.0, 0.0)
    UPDATED_POSITION = azlmbr.math.Vector3(10.0, 0.0, 0.0)
    NUM_NESTED_PREFABS_LEVELS = 3

    # Create nested prefabs at the root level. Also creates first instance
    entity = EditorEntity.create_editor_entity_at(CREATION_POSITION, name=NESTED_PREFABS_TEST_ENTITY_NAME)
    assert entity.id.IsValid(), "Couldn't create TestEntity"
    nested_prefabs, nested_prefab_instances = prefab_test_utils.create_linear_nested_prefabs(
        [entity], NESTED_PREFABS_FILE_NAME_PREFIX, NESTED_PREFABS_NAME_PREFIX, NUM_NESTED_PREFABS_LEVELS)
    prefab_test_utils.validate_linear_nested_prefab_instances_hierarchy(nested_prefab_instances)

    # Create a second instance
    nested_prefab_root = nested_prefabs[0]
    nested_prefab_instance_root_2 = nested_prefab_root.instantiate()
    assert nested_prefab_instance_root_2.is_valid(), "Failed to instantiate prefab"

    # Edit transform of the entity owned by the deepest nested instance of the first instance
    deepest_nested_prefab_instance_1 = nested_prefab_instances[NUM_NESTED_PREFABS_LEVELS-1]
    entity_of_deepest_nested_instance_1 = deepest_nested_prefab_instance_1.get_direct_child_entities()[0]
    get_transform_component_outcome = editor.EditorComponentAPIBus(
        bus.Broadcast, "GetComponentOfType", entity_of_deepest_nested_instance_1.id, globals.property.EditorTransformComponentTypeId
    )
    entity_transform_component = EditorComponent(globals.property.EditorTransformComponentTypeId)
    entity_transform_component.id = get_transform_component_outcome.GetValue()
    hydra.set_component_property_value(entity_transform_component.id, "Values|Translate", UPDATED_POSITION)

    PrefabWaiter.wait_for_propagation()

    # Find the entity owned by the deepest nested instance of the second instance
    children = nested_prefab_instance_root_2.get_direct_child_entities()
    while children:
        entity_of_deepest_nested_instance_2 = children[0]
        children = entity_of_deepest_nested_instance_2.get_children()

    # Validate that only the first prefab instance has changed
    entity_of_deepest_nested_instance_1.validate_world_translate_position(UPDATED_POSITION)
    entity_of_deepest_nested_instance_2.validate_world_translate_position(CREATION_POSITION)

    # Undo the override
    general.undo()
    PrefabWaiter.wait_for_propagation()

    # Validate the undo
    entity_of_deepest_nested_instance_1.validate_world_translate_position(CREATION_POSITION)
    entity_of_deepest_nested_instance_2.validate_world_translate_position(CREATION_POSITION)

    # Redo the override
    general.redo()
    PrefabWaiter.wait_for_propagation()

    # Validate the redo
    entity_of_deepest_nested_instance_1.validate_world_translate_position(UPDATED_POSITION)
    entity_of_deepest_nested_instance_2.validate_world_translate_position(CREATION_POSITION)

if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(EditEntity_UnderNestedInstance)
