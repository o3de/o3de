"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

def EditEntity_UnderNestedInstance():
    """
    Test description:
    - Creates two motorcycle prefab instances where each motorcycle has two wheel instances.
    - Puts focus on the first motorcycle instance and changes the position of one of the wheels.
    - Validates that after propagation, only one wheel of the focused motorcycle is changed.
    - Validates that after propagation, the wheel positions of the second motorcycle match those of the first.

    - Level
    -- Motorcycle (first instance) <-- focus
    --- Wheel (nested instance)
    ---- Wheel_Entity <-- change this entity's name to "Front_Wheel" and edit its position
    --- Wheel (nested instance)
    ---- Wheel_Entity <-- change this entity's name to "Back_Wheel"
    -- Motorcycle (second instance)
    --- Wheel (nested instance)
    ---- Wheel_Entity
    --- Wheel (nested instance)
    ---- Wheel_Entity
    """

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

    WHEEL_PREFAB_FILE_NAME = Path(__file__).stem + 'wheel_prefab'
    MOTORCYCLE_PREFAB_FILE_NAME = Path(__file__).stem + 'motorcycle_prefab'
    CREATION_POSITION = azlmbr.math.Vector3(0.0, 0.0, 0.0)
    UPDATED_POSITION = azlmbr.math.Vector3(10.0, 0.0, 0.0)

    # Create a wheel prefab from a wheel entity. Also creates first wheel instance
    wheel_entity = EditorEntity.create_editor_entity_at(CREATION_POSITION, "Wheel_Entity")
    wheel_prefab, wheel_instance_1 = Prefab.create_prefab([wheel_entity], WHEEL_PREFAB_FILE_NAME)

    # Create a second wheel instance
    wheel_instance_2 = wheel_prefab.instantiate()

    # Create a motorcycle prefab with two wheel instances. Also creates first motorcycle instance
    motorcycle_prefab, motorcycle_instance_1 = Prefab.create_prefab([wheel_instance_1.container_entity, wheel_instance_2.container_entity], MOTORCYCLE_PREFAB_FILE_NAME)

    # Create a second motorcycle instance
    motorcycle_instance_2 = motorcycle_prefab.instantiate()

    # Set focused prefab to first motorcycle instance
    motorcycle_instance_1.container_entity.focus_on_owning_prefab()

    # Change the names of the focused motorcycle's wheel entities
    wheel_instance_1_1 = motorcycle_instance_1.get_direct_child_entities()[0]
    front_wheel_entity = wheel_instance_1_1.get_children()[0]
    front_wheel_entity.set_name("Front_Wheel")
    wheel_instance_1_2 = motorcycle_instance_1.get_direct_child_entities()[1]
    back_wheel_entity = wheel_instance_1_2.get_children()[0]
    back_wheel_entity.set_name("Back_Wheel")

    # Change the position of the focused motorcycle's front wheel entity
    get_transform_component_outcome = editor.EditorComponentAPIBus(
        bus.Broadcast, "GetComponentOfType", front_wheel_entity.id, globals.property.EditorTransformComponentTypeId
    )
    entity_transform_component = EditorComponent(globals.property.EditorTransformComponentTypeId)
    entity_transform_component.id = get_transform_component_outcome.GetValue()
    hydra.set_component_property_value(entity_transform_component.id, "Values|Translate", UPDATED_POSITION)

    PrefabWaiter.wait_for_propagation()

    # Validate that the front wheels of both motorcycle instances have
    # the updated position, and that the back wheels remain unchanged
    front_wheels = EditorEntity.find_editor_entities(["Front_Wheel"])
    assert len(front_wheels) == 2, 'Expected to find a total of two front wheels.'
    for front_wheel in front_wheels:
        front_wheel.validate_world_translate_position(UPDATED_POSITION)
    back_wheels = EditorEntity.find_editor_entities(["Back_Wheel"])
    assert len(back_wheels) == 2, 'Expected to find a total of two back wheels.'
    for back_wheel in back_wheels:
        back_wheel.validate_world_translate_position(CREATION_POSITION)

    # Undo the override
    general.undo()
    PrefabWaiter.wait_for_propagation()

    # Validate the undo
    front_wheels = EditorEntity.find_editor_entities(["Front_Wheel"])
    for front_wheel in front_wheels:
        front_wheel.validate_world_translate_position(CREATION_POSITION)
    back_wheels = EditorEntity.find_editor_entities(["Back_Wheel"])
    for back_wheel in back_wheels:
        back_wheel.validate_world_translate_position(CREATION_POSITION)

    # Redo the override
    general.redo()
    PrefabWaiter.wait_for_propagation()

    # Validate the redo
    front_wheels = EditorEntity.find_editor_entities(["Front_Wheel"])
    for front_wheel in front_wheels:
        front_wheel.validate_world_translate_position(UPDATED_POSITION)
    back_wheels = EditorEntity.find_editor_entities(["Back_Wheel"])
    for back_wheel in back_wheels:
        back_wheel.validate_world_translate_position(CREATION_POSITION)

if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(EditEntity_UnderNestedInstance)
