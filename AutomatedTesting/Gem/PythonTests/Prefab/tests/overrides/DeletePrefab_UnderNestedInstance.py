"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

def DeletePrefab_UnderNestedInstance():
    """
    Test description:
    - Focuses on the "First_Car" instance.
    - Deletes the "Tire_Prefab" instance of the "Front_Wheel" under "First_Car".
    - Checks that there are 2 "Tire_Prefab" and 2 "Tire_Entity" and the deleted 
      instances are only in the "Front_Wheel" under both car instances.
    - Checks undo/redo correctness.
    - Checks that the deleted "Tire_Prefab" (and "Tire_Entity") should re-appear 
      after focusing on its owning instance (i.e. "Front_Wheel") in "First_Car".

    Hierarchy:
    - Level
      - First_Car                <-- focus on this prefab
        - Front_Wheel
          - Tire_Prefab          <-- delete this instance as override
            - Tire_Entity
        - Back_Wheel
          - Tire_Prefab
            - Tire_Entity
      - Second_Car
        - Front_Wheel
          - Tire_Prefab          <-- this should be deleted as well after propagation
            - Tire_Entity
        - Back_Wheel
          - Tire_Prefab
            - Tire_Entity
    """

    from editor_python_test_tools.editor_entity_utils import EditorEntity
    from editor_python_test_tools.prefab_utils import Prefab
    from editor_python_test_tools.wait_utils import PrefabWaiter

    import azlmbr.legacy.general as general
    import Prefab.tests.PrefabTestUtils as prefab_test_utils

    CAR_PREFAB_FILE_NAME = Path(__file__).stem + "_" + "car_prefab"
    WHEEL_PREFAB_FILE_NAME = Path(__file__).stem + "_" + "wheel_prefab"
    TIRE_PREFAB_FILE_NAME = Path(__file__).stem + "_" + "tire_prefab"

    FIRST_CAR_NAME = "First_Car"
    SECOND_CAR_NAME = "Second_Car"
    FRONT_WHEEL_NAME = "Front_Wheel"
    BACK_WHEEL_NAME = "Back_Wheel"
    TIRE_PREFAB_NAME = "Tire_Prefab"
    TIRE_ENTITY_NAME = "Tire_Entity"

    prefab_test_utils.open_base_tests_level()

    # Create a new entity at the root level.
    tire_entity = EditorEntity.create_editor_entity_at((100.0, 100.0, 100.0), TIRE_ENTITY_NAME)
    assert tire_entity.id.IsValid(), f"Failed to create new entity: {TIRE_ENTITY_NAME}."

    # Create a tire prefab from the tire entity.
    tire_prefab_entities = [tire_entity]
    tire_prefab, tire_instance = Prefab.create_prefab(tire_prefab_entities, TIRE_PREFAB_FILE_NAME, TIRE_PREFAB_NAME)
    assert tire_instance.is_valid(), f"Failed to instantiate instance: {TIRE_PREFAB_NAME}."

    # Create a wheel prefab and front wheel instance from the tire instance.
    wheel_prefab_entities = [tire_instance.container_entity]
    wheel_prefab, front_wheel_instance = Prefab.create_prefab(wheel_prefab_entities, WHEEL_PREFAB_FILE_NAME, \
        FRONT_WHEEL_NAME)
    assert front_wheel_instance.is_valid(), f"Failed to instantiate instance: {FRONT_WHEEL_NAME}."

    # Create a back wheel instance.
    back_wheel_instance = wheel_prefab.instantiate(name=BACK_WHEEL_NAME)
    assert back_wheel_instance.is_valid(), f"Failed to instantiate instance: {BACK_WHEEL_NAME}."
    
    # Create a car prefab and the first car instance from two wheels.
    car_prefab_entities = [front_wheel_instance.container_entity, back_wheel_instance.container_entity]
    car_prefab, car_instance_1 = Prefab.create_prefab(car_prefab_entities, CAR_PREFAB_FILE_NAME, FIRST_CAR_NAME)
    assert car_instance_1.is_valid(), f"Failed to instantiate instance: {FIRST_CAR_NAME}."

    # Create a second car instance.
    car_instance_2 = car_prefab.instantiate(name=SECOND_CAR_NAME)
    assert car_instance_2.is_valid(), f"Failed to instantiate instance: {SECOND_CAR_NAME}."
    
    # Focus on the first car instance.
    car_instance_1.container_entity.focus_on_owning_prefab()

    # Delete the tire instance in the front wheel instance in the first car instance.
    wheel_container_entities_in_car_instance_1 = car_instance_1.get_direct_child_entities()
    filtered_wheel_list = list(filter(lambda wheel_container_entity: \
        wheel_container_entity.get_name() == FRONT_WHEEL_NAME, \
        wheel_container_entities_in_car_instance_1))
    assert len(filtered_wheel_list) == 1, f"Found {len(filtered_wheel_list)} {FRONT_WHEEL_NAME}. " \
                                          f"Expected 1 {FRONT_WHEEL_NAME} in {FIRST_CAR_NAME} to delete."

    tire_container_entity_in_front_wheel_in_car_instance_1 = filtered_wheel_list[0].get_children()[0]
    tire_container_entity_in_front_wheel_in_car_instance_1.delete()

    # Wait till prefab propagation finishes before validating deletion.
    PrefabWaiter.wait_for_propagation()

    # Validate after instance deletion.
    prefab_test_utils.validate_count_for_named_editor_entity(TIRE_PREFAB_NAME, 2)
    prefab_test_utils.validate_count_for_named_editor_entity(TIRE_ENTITY_NAME, 2)

    prefab_test_utils.validate_child_count_for_named_editor_entity(FRONT_WHEEL_NAME, 0)
    prefab_test_utils.validate_child_count_for_named_editor_entity(BACK_WHEEL_NAME, 1)

    # Validate undo on instance deletion.
    general.undo()
    PrefabWaiter.wait_for_propagation()
    
    prefab_test_utils.validate_count_for_named_editor_entity(TIRE_PREFAB_NAME, 4)
    prefab_test_utils.validate_count_for_named_editor_entity(TIRE_ENTITY_NAME, 4)

    prefab_test_utils.validate_child_count_for_named_editor_entity(FRONT_WHEEL_NAME, 1)
    prefab_test_utils.validate_child_count_for_named_editor_entity(BACK_WHEEL_NAME, 1)

    # Validate redo on instance deletion.
    general.redo()
    PrefabWaiter.wait_for_propagation()

    prefab_test_utils.validate_count_for_named_editor_entity(TIRE_PREFAB_NAME, 2)
    prefab_test_utils.validate_count_for_named_editor_entity(TIRE_ENTITY_NAME, 2)
    
    prefab_test_utils.validate_child_count_for_named_editor_entity(FRONT_WHEEL_NAME, 0)
    prefab_test_utils.validate_child_count_for_named_editor_entity(BACK_WHEEL_NAME, 1)

    # Focus on first wheel instance in first car instance. Deleted tire instance should appear in Prefab Edit Mode.
    front_wheel_container_entities = EditorEntity.find_editor_entities([FRONT_WHEEL_NAME])
    filtered_front_wheel_list = list(filter(lambda front_wheel: \
        front_wheel.get_parent_id() == car_instance_1.container_entity.id, \
        front_wheel_container_entities))
    assert len(filtered_front_wheel_list) == 1, f"Found {len(filtered_front_wheel_list)} {FRONT_WHEEL_NAME}. " \
                                                f"Expected 1 {FRONT_WHEEL_NAME} in {FIRST_CAR_NAME} to focus on."
    front_wheel_in_car_instance_1 = filtered_front_wheel_list[0]

    front_wheel_in_car_instance_1.focus_on_owning_prefab()
    PrefabWaiter.wait_for_propagation() # propagate source template changes after focusing on

    prefab_test_utils.check_entity_children_count(front_wheel_in_car_instance_1.id, 1)

    # Note: This should be 3 because the tire instance inside the front wheel under the second car is still deleted.
    prefab_test_utils.validate_count_for_named_editor_entity(TIRE_PREFAB_NAME, 3)
    prefab_test_utils.validate_count_for_named_editor_entity(TIRE_ENTITY_NAME, 3)


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(DeletePrefab_UnderNestedInstance)
