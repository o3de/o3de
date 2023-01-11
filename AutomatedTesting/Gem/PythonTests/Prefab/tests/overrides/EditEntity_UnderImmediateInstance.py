"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


def EditEntity_UnderImmediateInstance():
    """
    Test description:
    - Creates a test prefab in the level, and instantiates it twice
    - Edits the transform as an override on only the entity under the 2nd instance
    - Validates overrides/undo/redo/revert overrides all function properly
    - Adds a new component to the test prefab
    - Adds another component as an override only the entity under the 2nd instance
    - Validates overrides/undo/redo/revert overrides all function properly
    - Removes a component as an override only from the entity under the 1st instance
    - Validates overrides/undo/redo/revert overrides all function properly
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

    # Move the car entity to a new position.
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
    prefab_test_utils.validate_expected_override_status(car_entity_of_instance_1, False)
    prefab_test_utils.validate_expected_override_status(car_entity_of_instance_2, True)

    # Undo the override
    general.undo()
    PrefabWaiter.wait_for_propagation()

    # Validate the undo
    car_entity_of_instance_1.validate_world_translate_position(CREATION_POSITION)
    car_entity_of_instance_2.validate_world_translate_position(CREATION_POSITION)
    prefab_test_utils.validate_expected_override_status(car_entity_of_instance_1, False)
    prefab_test_utils.validate_expected_override_status(car_entity_of_instance_2, False)

    # Redo the override
    general.redo()
    PrefabWaiter.wait_for_propagation()

    # Validate the redo
    car_entity_of_instance_1.validate_world_translate_position(CREATION_POSITION)
    car_entity_of_instance_2.validate_world_translate_position(UPDATED_POSITION)
    prefab_test_utils.validate_expected_override_status(car_entity_of_instance_1, False)
    prefab_test_utils.validate_expected_override_status(car_entity_of_instance_2, True)

    # Revert the override, and validate
    car_entity_of_instance_2.revert_overrides()
    PrefabWaiter.wait_for_propagation()
    car_entity_of_instance_1.validate_world_translate_position(CREATION_POSITION)
    car_entity_of_instance_2.validate_world_translate_position(CREATION_POSITION)
    prefab_test_utils.validate_expected_override_status(car_entity_of_instance_1, False)
    prefab_test_utils.validate_expected_override_status(car_entity_of_instance_2, False)

    # Focus on the owning prefab and add a new component to both instances
    car_instance_1.container_entity.focus_on_owning_prefab()
    car_entity_of_instance_1.add_component("Mesh")

    # Focus on the level prefab, then add a different component as an override to entity in the second instance
    level_entity = EditorEntity(car_instance_1.container_entity.get_parent_id())
    level_entity.focus_on_owning_prefab()
    car_entity_of_instance_2.add_component("Comment")

    # Validate each instance has the expected components: Mesh on both, and Comment only on the 2nd instance
    prefab_test_utils.validate_expected_components(car_entity_of_instance_1, expected_components=["Mesh"],
                                                   unexpected_components=["Comment"])
    prefab_test_utils.validate_expected_components(car_entity_of_instance_2, expected_components=["Mesh", "Comment"])
    prefab_test_utils.validate_expected_override_status(car_entity_of_instance_1, False)
    prefab_test_utils.validate_expected_override_status(car_entity_of_instance_2, True)

    # Undo each override
    general.undo()  # Undo Comment component add
    general.undo()  # Undo focus on level instance
    general.undo()  # Undo Mesh component add
    PrefabWaiter.wait_for_propagation()

    # Validate each instance has reverted the addition of the new components with Undo
    prefab_test_utils.validate_expected_components(car_entity_of_instance_1, unexpected_components=["Mesh", "Comment"])
    prefab_test_utils.validate_expected_components(car_entity_of_instance_2, unexpected_components=["Mesh", "Comment"])
    prefab_test_utils.validate_expected_override_status(car_entity_of_instance_1, False)
    prefab_test_utils.validate_expected_override_status(car_entity_of_instance_2, False)

    # Redo each override
    general.redo()  # Redo Mesh component add
    general.redo()  # Redo focus on level instance
    general.redo()  # Redo Comment component add
    PrefabWaiter.wait_for_propagation()

    # Validate each instance has the expected components: Mesh on both, and Comment only on the 2nd instance
    prefab_test_utils.validate_expected_components(car_entity_of_instance_1, expected_components=["Mesh"],
                                                   unexpected_components=["Comment"])
    prefab_test_utils.validate_expected_components(car_entity_of_instance_2, expected_components=["Mesh", "Comment"])
    prefab_test_utils.validate_expected_override_status(car_entity_of_instance_1, False)
    prefab_test_utils.validate_expected_override_status(car_entity_of_instance_2, True)

    # Focus on the level prefab so overrides can be applied, then remove the Mesh component from the first instance
    level_entity.focus_on_owning_prefab()
    car_entity_of_instance_1.remove_component("Mesh")
    PrefabWaiter.wait_for_propagation()

    # Validate both instance entities now have overrides applied, and each has the proper components
    prefab_test_utils.validate_expected_components(car_entity_of_instance_1, unexpected_components=["Mesh", "Comment"])
    prefab_test_utils.validate_expected_components(car_entity_of_instance_2, expected_components=["Mesh", "Comment"])
    prefab_test_utils.validate_expected_override_status(car_entity_of_instance_1, True)
    prefab_test_utils.validate_expected_override_status(car_entity_of_instance_2, True)

    # Undo the Mesh component removal override
    general.undo()
    PrefabWaiter.wait_for_propagation()

    # Validate expected overrides and components after Undo
    prefab_test_utils.validate_expected_components(car_entity_of_instance_1, expected_components=["Mesh"],
                                                   unexpected_components=["Comment"])
    prefab_test_utils.validate_expected_components(car_entity_of_instance_2, expected_components=["Mesh", "Comment"])
    prefab_test_utils.validate_expected_override_status(car_entity_of_instance_1, False)
    prefab_test_utils.validate_expected_override_status(car_entity_of_instance_2, True)

    # Redo the Mesh component removal override
    general.redo()
    PrefabWaiter.wait_for_propagation()

    # Validate expected overrides and components after Redo
    prefab_test_utils.validate_expected_components(car_entity_of_instance_1, unexpected_components=["Mesh", "Comment"])
    prefab_test_utils.validate_expected_components(car_entity_of_instance_2, expected_components=["Mesh", "Comment"])
    prefab_test_utils.validate_expected_override_status(car_entity_of_instance_1, True)
    prefab_test_utils.validate_expected_override_status(car_entity_of_instance_2, True)

    # Revert overrides on both entities
    car_entity_of_instance_1.revert_overrides()
    car_entity_of_instance_2.revert_overrides()
    PrefabWaiter.wait_for_propagation()

    # Validate expected overrides and component after reverting
    prefab_test_utils.validate_expected_components(car_entity_of_instance_1, expected_components=["Mesh"],
                                                   unexpected_components=["Comment"])
    prefab_test_utils.validate_expected_components(car_entity_of_instance_2, expected_components=["Mesh"],
                                                   unexpected_components=["Comment"])
    prefab_test_utils.validate_expected_override_status(car_entity_of_instance_1, False)
    prefab_test_utils.validate_expected_override_status(car_entity_of_instance_2, False)


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(EditEntity_UnderImmediateInstance)
