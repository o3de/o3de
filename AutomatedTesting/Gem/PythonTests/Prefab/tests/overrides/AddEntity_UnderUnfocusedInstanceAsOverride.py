"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

def Undo_AddEntity():
    from editor_python_test_tools.wait_utils import PrefabWaiter

    # One of the undos is for entity selection.
    azlmbr.legacy.general.undo()
    PrefabWaiter.wait_for_propagation()
    azlmbr.legacy.general.undo()
    PrefabWaiter.wait_for_propagation()

def Redo_AddEntity():
    from editor_python_test_tools.wait_utils import PrefabWaiter

    # One of the redos is for entity selection.
    azlmbr.legacy.general.redo()
    PrefabWaiter.wait_for_propagation()
    azlmbr.legacy.general.redo()
    PrefabWaiter.wait_for_propagation()
    

def AddEntity_UnderUnfocusedInstanceAsOverride():
    """
    Test description:
    - Create a car prefab/instance with the following hierarchy, and focus on the car:
        CAR (car instance, focused)
        |- AXLE (axle instance)
           |- LeftWheel (wheel instance)
              |- WheelEntity
           |- RightWheel (wheel instance)
              |- WheelEntity

    - Create a new entity 'TireEntity' and add it under 'LeftWheel' as override of car instance.
    - Undo/Redo adding 'TireEntity' under 'LeftWheel'.
    - Check that 'TireEntity' is correctly added without accidentally modifying another wheel instance 'RightWheel' during prefab propagations.
    """

    from editor_python_test_tools.editor_entity_utils import EditorEntity
    from editor_python_test_tools.prefab_utils import Prefab
    from editor_python_test_tools.wait_utils import PrefabWaiter

    import Prefab.tests.PrefabTestUtils as prefab_test_utils

    prefab_test_utils.open_base_tests_level()

    from pathlib import Path
    WHEEL_PREFAB_FILE_NAME = Path(__file__).stem + '_' + 'wheel_prefab'    
    AXLE_PREFAB_FILE_NAME = Path(__file__).stem + '_' + 'axle_prefab'
    CAR_PREFAB_FILE_NAME = Path(__file__).stem + '_' + 'car_prefab'

    # Create wheel/axle/car prefabs and instances, and then focus on car instance.    
    LEFT_WHEEL_INSTANCE_NAME = "LeftWheel"
    RIGHT_WHEEL_INSTANCE_NAME = "RightWheel"
    WHEEL_ENTITY_NAME = "WheelEntity"

    wheel_entity = EditorEntity.create_editor_entity(name=WHEEL_ENTITY_NAME)
    assert wheel_entity.id.IsValid(), f"Couldn't create entity '{WHEEL_ENTITY_NAME}'"

    wheel_prefab, left_wheel_instance = Prefab.create_prefab([wheel_entity], WHEEL_PREFAB_FILE_NAME, prefab_instance_name=LEFT_WHEEL_INSTANCE_NAME)
    right_wheel_instance = wheel_prefab.instantiate(name=RIGHT_WHEEL_INSTANCE_NAME)
    
    AXLE_INSTANCE_NAME = "AXLE"
    _, axle_instance = Prefab.create_prefab(
      [left_wheel_instance.container_entity, right_wheel_instance.container_entity], AXLE_PREFAB_FILE_NAME, prefab_instance_name=AXLE_INSTANCE_NAME)
    
    CAR_INSTANCE_NAME = "CAR"
    _, car_instance = Prefab.create_prefab([axle_instance.container_entity], CAR_PREFAB_FILE_NAME, prefab_instance_name=CAR_INSTANCE_NAME)
    car_instance.container_entity.focus_on_owning_prefab()
    
    # Find the container entity of 'LeftWheel', creates a new entity 'TireEntity', and adds the new entity under 'LeftWheel'.
    left_wheel_instance_container_entity = EditorEntity.find_editor_entity(entity_name=LEFT_WHEEL_INSTANCE_NAME, must_be_unique=True)
    assert left_wheel_instance_container_entity.id.IsValid(), f"Couldn't find valid entity '{LEFT_WHEEL_INSTANCE_NAME}'"

    TIRE_ENTITY_NAME = "TireEntity"
    tire_entity = EditorEntity.create_editor_entity(parent_id=left_wheel_instance_container_entity.id, name=TIRE_ENTITY_NAME)
    # Wait till prefab propagation finishes before validation.
    PrefabWaiter.wait_for_propagation()

    # Check if 'TireEntity' is added under 'LeftWheel' only correctly. 
    assert tire_entity.id.IsValid(), f"Couldn't create entity '{TIRE_ENTITY_NAME}'' under prefab instance '{LEFT_WHEEL_INSTANCE_NAME}'"
    assert tire_entity.get_name() == TIRE_ENTITY_NAME, f"Entity '{tire_entity.get_name()}''s name should be {TIRE_ENTITY_NAME}"
    assert tire_entity.get_parent_id() == left_wheel_instance_container_entity.id, f"Entity '{LEFT_WHEEL_INSTANCE_NAME}' should be the parent of entity '{TIRE_ENTITY_NAME}'"
    prefab_test_utils.validate_expected_override_status(tire_entity, True)

    child_entity_ids = left_wheel_instance_container_entity.get_children()
    assert len(child_entity_ids) == 2, f"{len(child_entity_ids)} child entities found under entity '{LEFT_WHEEL_INSTANCE_NAME}'" \
                                              f" after add-entity operation, when there should have been 2 child entities"

    right_wheel_instance_container_entity = EditorEntity.find_editor_entity(entity_name=RIGHT_WHEEL_INSTANCE_NAME, must_be_unique=True)
    assert right_wheel_instance_container_entity.id.IsValid(), f"Couldn't find valid entity '{RIGHT_WHEEL_INSTANCE_NAME}'"

    child_entity_ids = right_wheel_instance_container_entity.get_children()
    assert len(child_entity_ids) == 1, f"{len(child_entity_ids)} child entities found under entity '{RIGHT_WHEEL_INSTANCE_NAME}'" \
                                              f" after add-entity operation, when there should have been 1 child entity"

    # Test undo/redo on adding 'TireEntity' under 'LeftWheel'.
    Undo_AddEntity()

    left_wheel_instance_container_entity = EditorEntity.find_editor_entity(entity_name=LEFT_WHEEL_INSTANCE_NAME, must_be_unique=True)
    assert left_wheel_instance_container_entity.id.IsValid(), f"Couldn't find valid entity '{LEFT_WHEEL_INSTANCE_NAME}'"

    child_entity_ids = left_wheel_instance_container_entity.get_children()
    assert len(child_entity_ids) == 1, f"{len(child_entity_ids)} child entities found under entity '{LEFT_WHEEL_INSTANCE_NAME}'" \
                                              f" after Undo operation, when there should have been 1 child entity"
    
    tire_entities = EditorEntity.find_editor_entities(entity_names=[TIRE_ENTITY_NAME])
    assert len(tire_entities) == 0, f"{len(tire_entities)} '{TIRE_ENTITY_NAME}' entities exist" \
                                              f" after Undo operation, when there shouldn't have been any"

    Redo_AddEntity()
    
    left_wheel_instance_container_entity = EditorEntity.find_editor_entity(entity_name=LEFT_WHEEL_INSTANCE_NAME, must_be_unique=True)
    assert left_wheel_instance_container_entity.id.IsValid(), f"Couldn't find valid entity '{LEFT_WHEEL_INSTANCE_NAME}'"

    tire_entity = EditorEntity.find_editor_entity(entity_name=TIRE_ENTITY_NAME, must_be_unique=True)
    assert tire_entity.id.IsValid(), f"Couldn't find valid entity '{TIRE_ENTITY_NAME}'"

    assert tire_entity.get_parent_id() == left_wheel_instance_container_entity.id, f"Entity '{LEFT_WHEEL_INSTANCE_NAME}' should be the parent of entity '{TIRE_ENTITY_NAME}'"

    child_entity_ids = left_wheel_instance_container_entity.get_children()
    assert len(child_entity_ids) == 2, f"{len(child_entity_ids)} child entities found under entity '{LEFT_WHEEL_INSTANCE_NAME}'" \
                                              f" after Redo operation, when there should have been 2 child entity"

    right_wheel_instance_container_entity = EditorEntity.find_editor_entity(entity_name=RIGHT_WHEEL_INSTANCE_NAME, must_be_unique=True)                                     
    assert right_wheel_instance_container_entity.id.IsValid(), f"Couldn't find valid entity '{RIGHT_WHEEL_INSTANCE_NAME}'"

    child_entity_ids = right_wheel_instance_container_entity.get_children()
    assert len(child_entity_ids) == 1, f"{len(child_entity_ids)} child entities found under entity '{RIGHT_WHEEL_INSTANCE_NAME}'" \
                                              f" after Redo operation, when there should have been 1 child entity"
    # Revert the re-applied overrides
    tire_entity.revert_overrides()
    PrefabWaiter.wait_for_propagation()

    # Validate the revert
    tire_entities = EditorEntity.find_editor_entities(entity_names=[TIRE_ENTITY_NAME])
    assert len(tire_entities) == 0, f"Expected 0 '{TIRE_ENTITY_NAME}' entities after Revert Overrides operation, but " \
                                    f"found {len(tire_entities)}."


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(AddEntity_UnderUnfocusedInstanceAsOverride)
