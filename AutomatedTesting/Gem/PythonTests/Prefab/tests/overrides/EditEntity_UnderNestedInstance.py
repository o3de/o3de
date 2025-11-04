"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


def EditEntity_UnderNestedInstance():
    """
    Test description:
    - Creates two motorcycle prefab instances where each motorcycle has two wheel instances with Mesh components.
    - Puts focus on the first motorcycle instance, changes the position of one of the wheels, and adds a Comment
    component.
    - Validates that after propagation, only one wheel of the focused motorcycle is changed.
    - Validates that after propagation, the wheel positions and expected components of the second motorcycle match those
    of the first.
    - Validates Undo/Redo of all the overrides.
    - Validates revert overrides of all the overrides.

    - Level
    -- Motorcycle (first instance) <-- focus
    --- Wheel (nested instance)
    ---- Wheel_Entity <-- change this entity's name to "Front_Wheel", edit its position, and add a "Comment" component.
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

    WHEEL_PREFAB_FILE_NAME = Path(__file__).stem + '_wheel_prefab'
    MOTORCYCLE_PREFAB_FILE_NAME = Path(__file__).stem + '_motorcycle_prefab'
    CREATION_POSITION = azlmbr.math.Vector3(0.0, 0.0, 0.0)
    UPDATED_POSITION = azlmbr.math.Vector3(10.0, 0.0, 0.0)

    # Create a wheel prefab/instance from a wheel entity
    wheel_entity = EditorEntity.create_editor_entity_at(CREATION_POSITION, "Wheel_Entity")
    wheel_prefab, wheel_instance_1 = Prefab.create_prefab([wheel_entity], WHEEL_PREFAB_FILE_NAME)
    wheel_entity = wheel_instance_1.get_direct_child_entities()[0]

    # Focus on the new instance and add a component, then focus back on the level prefab
    wheel_instance_1.container_entity.focus_on_owning_prefab()
    wheel_entity.add_component("Mesh")
    level_entity = EditorEntity(wheel_instance_1.container_entity.get_parent_id())
    level_entity.focus_on_owning_prefab()

    # Create a second wheel instance
    wheel_instance_2 = wheel_prefab.instantiate()

    # Create a motorcycle prefab with two wheel instances. Also creates first motorcycle instance
    motorcycle_prefab, motorcycle_instance_1 = Prefab.create_prefab(
        [wheel_instance_1.container_entity, wheel_instance_2.container_entity], MOTORCYCLE_PREFAB_FILE_NAME)

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

    # Change the position of the focused motorcycle's front wheel entity, and add another component as overrides.
    get_transform_component_outcome = editor.EditorComponentAPIBus(
        bus.Broadcast, "GetComponentOfType", front_wheel_entity.id, globals.property.EditorTransformComponentTypeId
    )
    entity_transform_component = EditorComponent(globals.property.EditorTransformComponentTypeId)
    entity_transform_component.id = get_transform_component_outcome.GetValue()
    hydra.set_component_property_value(entity_transform_component.id, "Values|Translate", UPDATED_POSITION)
    front_wheel_entity.add_component("Comment")

    PrefabWaiter.wait_for_propagation()

    # Validate that the front wheels of both motorcycle instances have
    # the updated position, and that the back wheels remain unchanged
    front_wheels = EditorEntity.find_editor_entities(["Front_Wheel"])
    assert len(front_wheels) == 2, "Expected to find a total of two front wheels."
    for front_wheel in front_wheels:
        front_wheel.validate_world_translate_position(UPDATED_POSITION)
    back_wheels = EditorEntity.find_editor_entities(["Back_Wheel"])
    assert len(back_wheels) == 2, "Expected to find a total of two back wheels."
    for back_wheel in back_wheels:
        back_wheel.validate_world_translate_position(CREATION_POSITION)

    # Validate the expected entities are flagged for overrides on each Motorcycle instance, and have the expected
    # components

    # Instance 1, which is already in focus
    prefab_test_utils.validate_expected_override_status(front_wheel_entity, True)
    prefab_test_utils.validate_expected_components(front_wheel_entity, expected_components=["Mesh", "Comment"])
    prefab_test_utils.validate_expected_override_status(back_wheel_entity, True)
    prefab_test_utils.validate_expected_components(back_wheel_entity, expected_components=["Mesh"],
                                                   unexpected_components=["Comment"])

    # Focus instance 2, and validate
    motorcycle_instance_2.container_entity.focus_on_owning_prefab()
    front_wheel_entity_2 = motorcycle_instance_2.get_child_entity_by_name("Front_Wheel")
    prefab_test_utils.validate_expected_components(front_wheel_entity_2, expected_components=["Mesh", "Comment"])
    back_wheel_entity_2 = motorcycle_instance_2.get_child_entity_by_name("Back_Wheel")
    prefab_test_utils.validate_expected_components(back_wheel_entity_2, expected_components=["Mesh"],
                                                   unexpected_components=["Comment"])
    prefab_test_utils.validate_expected_override_status(front_wheel_entity_2, True)
    prefab_test_utils.validate_expected_override_status(back_wheel_entity_2, True)

    # Undo the override
    general.undo()  # Undo the focus change
    general.undo()  # Undo the Comment component addition
    general.undo()  # Undo the transform override
    PrefabWaiter.wait_for_propagation()

    # Validate the undo
    front_wheels = EditorEntity.find_editor_entities(["Front_Wheel"])
    for front_wheel in front_wheels:
        front_wheel.validate_world_translate_position(CREATION_POSITION)
    back_wheels = EditorEntity.find_editor_entities(["Back_Wheel"])
    for back_wheel in back_wheels:
        back_wheel.validate_world_translate_position(CREATION_POSITION)

    prefab_test_utils.validate_expected_components(front_wheel_entity, expected_components=["Mesh"],
                                                   unexpected_components=["Comment"])
    prefab_test_utils.validate_expected_components(back_wheel_entity, expected_components=["Mesh"],
                                                   unexpected_components=["Comment"])

    # Redo the override
    general.redo()  # Redo the transform override
    general.redo()  # Redo the Comment component addition
    PrefabWaiter.wait_for_propagation()

    # Validate the redo
    front_wheels = EditorEntity.find_editor_entities(["Front_Wheel"])
    for front_wheel in front_wheels:
        front_wheel.validate_world_translate_position(UPDATED_POSITION)
    back_wheels = EditorEntity.find_editor_entities(["Back_Wheel"])
    for back_wheel in back_wheels:
        back_wheel.validate_world_translate_position(CREATION_POSITION)

    # Revert all overrides on each entity
    front_wheel_entity.revert_overrides()
    back_wheel_entity.revert_overrides()
    PrefabWaiter.wait_for_propagation()

    # Validate the revert. All wheel entities should now have the initial name "Wheel_Entity" and be at the
    # initial position.
    wheels = EditorEntity.find_editor_entities(["Wheel_Entity"])
    assert len(wheels) == 4, f"Expected to find a total of four wheels. Found {len(wheels)}"
    for wheel in wheels:
        wheel.validate_world_translate_position(CREATION_POSITION)
        prefab_test_utils.validate_expected_override_status(wheel, False)


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(EditEntity_UnderNestedInstance)
