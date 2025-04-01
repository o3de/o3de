"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

def DeleteEntity_UnderImmediateInstance():
    """
    Test description:
    - Deletes the "Tire_Entity" of the "First_Car" instance.
    - Checks that the "Tire_Entity" is deleted only in "First_Car" instance.
    - Checks undo/redo correctness.
    - Checks that the deleted "Tire_Entity" should re-appear after focusing on
      its owning instance (i.e. "First_Car").

    Hierarchy:
    - Level                   <-- focus on this prefab
      - First_Car
        - Tire_Entity         <-- delete this entity as override
      - Second_Car
        - Tire_Entity         <-- this entity is unchanged
    """

    from editor_python_test_tools.editor_entity_utils import EditorEntity
    from editor_python_test_tools.prefab_utils import Prefab
    from editor_python_test_tools.wait_utils import PrefabWaiter

    import azlmbr.legacy.general as general
    import Prefab.tests.PrefabTestUtils as prefab_test_utils

    CAR_PREFAB_FILE_NAME = Path(__file__).stem + "_" + "car_prefab"

    FIRST_CAR_NAME = "First_Car"
    SECOND_CAR_NAME = "Second_Car"
    TIRE_ENTITY_NAME = "Tire_Entity"

    prefab_test_utils.open_base_tests_level()

    # Create a new entity at the root level.
    tire_entity = EditorEntity.create_editor_entity_at((100.0, 100.0, 100.0), TIRE_ENTITY_NAME)
    assert tire_entity.id.IsValid(), f"Failed to create new entity: {TIRE_ENTITY_NAME}."

    # Create a car prefab and the first car instance from the tire entity.
    car_prefab_entities = [tire_entity]
    car_prefab, car_instance_1 = Prefab.create_prefab(car_prefab_entities, CAR_PREFAB_FILE_NAME, FIRST_CAR_NAME)
    assert car_instance_1.is_valid(), f"Failed to instantiate instance: {FIRST_CAR_NAME}."

    # Create the second car instance.
    car_instance_2 = car_prefab.instantiate(name=SECOND_CAR_NAME)
    assert car_instance_2.is_valid(), f"Failed to instantiate instance: {SECOND_CAR_NAME}."
    
    # Delete the tire entity in the first car. Note: Level prefab is currently being focused by default during deletion.
    tire_entities_in_car_instance_1 = car_instance_1.get_direct_child_entities()
    tire_entity_in_car_instance_1 = tire_entities_in_car_instance_1[0]
    tire_entity_in_car_instance_1.delete()

    # Wait till prefab propagation finishes before validating deletion.
    PrefabWaiter.wait_for_propagation()

    # Validate after entity deletion.
    prefab_test_utils.validate_count_for_named_editor_entity(TIRE_ENTITY_NAME, 1)
    prefab_test_utils.check_entity_children_count(car_instance_1.container_entity.id, 0)
    prefab_test_utils.check_entity_children_count(car_instance_2.container_entity.id, 1)

    # Validate undo on entity deletion.
    general.undo()
    PrefabWaiter.wait_for_propagation()

    prefab_test_utils.validate_count_for_named_editor_entity(TIRE_ENTITY_NAME, 2)
    prefab_test_utils.check_entity_children_count(car_instance_1.container_entity.id, 1)
    prefab_test_utils.check_entity_children_count(car_instance_2.container_entity.id, 1)

    # Validate redo on entity deletion.
    general.redo()
    PrefabWaiter.wait_for_propagation()

    prefab_test_utils.validate_count_for_named_editor_entity(TIRE_ENTITY_NAME, 1)
    prefab_test_utils.check_entity_children_count(car_instance_1.container_entity.id, 0)
    prefab_test_utils.check_entity_children_count(car_instance_2.container_entity.id, 1)

    # Focus on the first car instance. Deleted tire entity should appear in Prefab Edit Mode.
    car_instance_1.container_entity.focus_on_owning_prefab()

    PrefabWaiter.wait_for_propagation() # propagate source template changes after focusing on
    prefab_test_utils.check_entity_children_count(car_instance_1.container_entity.id, 1)

    # Note: This should be 2 because the tire entity inside the second car is not deleted.
    prefab_test_utils.validate_count_for_named_editor_entity(TIRE_ENTITY_NAME, 2)


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(DeleteEntity_UnderImmediateInstance)
