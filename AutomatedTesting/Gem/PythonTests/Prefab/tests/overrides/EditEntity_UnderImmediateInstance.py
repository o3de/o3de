"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

def EditEntity_UnderImmediateInstance():
    """
    Test description:
    - Creates a "Car" prefab containing one entity called "Car_Entity".
    - Instantiates the "Car" prefab twice under the level prefab.
    - Edits the "Car_Entity" of the second "Car" instance.
    - Checks that after propagation, the "Car_Entity" of the second "Car" instance contains the correct edits.
    - Checks that after propagation, the "Car_Entity" of the first "Car" instance does not contain any edits.

    - Level (Focused prefab)
    -- Car (first instance)
    --- Car_Entity
    -- Car (second instance)
    --- Car_Entity <-- edit this entity's transform
    """

    from editor_python_test_tools.editor_entity_utils import EditorEntity, EditorComponent
    from editor_python_test_tools.prefab_utils import Prefab
    from editor_python_test_tools.wait_utils import PrefabWaiter
    import azlmbr.legacy.general as general
    import Prefab.tests.PrefabTestUtils as prefab_test_utils
    import azlmbr.bus as bus
    import azlmbr.editor as editor
    import azlmbr.globals as globals
    import editor_python_test_tools.hydra_editor_utils as hydra

    CAR_PREFAB_FILE_NAME = Path(__file__).stem + 'car_prefab'
    CREATION_POSITION = azlmbr.math.Vector3(0.0, 0.0, 0.0)
    UPDATED_POSITION = azlmbr.math.Vector3(10.0, 0.0, 0.0)

    prefab_test_utils.open_base_tests_level()

    # Create a new entity at the root level
    car_entity = EditorEntity.create_editor_entity_at(CREATION_POSITION, "Car_Entity")
    car_prefab_entities = [car_entity]

    # Create a prefab from the new entity. Also creates first instance
    car_prefab, car_instance_1 = Prefab.create_prefab(car_prefab_entities, CAR_PREFAB_FILE_NAME)

    # Create a second instance
    car_instance_2 = car_prefab.instantiate()
    assert car_instance_2.is_valid(), "Failed to instantiate prefab"

    # Edit the second instance by changing the transform of the entity it owns
    car_entity_of_instance_2 = car_instance_2.get_direct_child_entities()[0]

    # Move the car entity to a new postion.
    get_transform_component_outcome = editor.EditorComponentAPIBus(
        bus.Broadcast, "GetComponentOfType", car_entity_of_instance_2.id, globals.property.EditorTransformComponentTypeId
    )
    car_transform_component = EditorComponent(globals.property.EditorTransformComponentTypeId)
    car_transform_component.id = get_transform_component_outcome.GetValue()
    hydra.set_component_property_value(car_transform_component.id, "Values|Translate", UPDATED_POSITION)

    PrefabWaiter.wait_for_propagation()

    # Validate that only the second prefab instance has changed
    car_entity_of_instance_1 = car_instance_1.get_direct_child_entities()[0]
    car_entity_of_instance_1.validate_world_translate_position(CREATION_POSITION)
    car_entity_of_instance_2.validate_world_translate_position(UPDATED_POSITION)

    # Undo the override
    general.undo()
    PrefabWaiter.wait_for_propagation()

    # Validate the undo
    car_entity_of_instance_1.validate_world_translate_position(CREATION_POSITION)
    car_entity_of_instance_2.validate_world_translate_position(CREATION_POSITION)

    # Redo the override
    general.redo()
    PrefabWaiter.wait_for_propagation()

    # Validate the redo
    car_entity_of_instance_1.validate_world_translate_position(CREATION_POSITION)
    car_entity_of_instance_2.validate_world_translate_position(UPDATED_POSITION)

if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(EditEntity_UnderImmediateInstance)
