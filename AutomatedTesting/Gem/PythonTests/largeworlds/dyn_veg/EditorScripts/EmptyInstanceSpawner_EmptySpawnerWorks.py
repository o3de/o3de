"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


class BehaviorContextTests:
    spawner_initialized = (
        "Successfully initialized an Empty Instance Spawner",
        "Failed to initialize an Empty Instance Spawner"
    )
    desc_spawnertype_sets_spawner = (
        "Setting spawnerType sets spawner too",
        "Setting spawnerType failed to set spawner to expected value"
    )
    desc_spawner_sets_spawnertype = (
        "Setting spawner sets spawnerType too",
        "Setting spawner failed to set spawnerType to expected value"
    )


class PropertyTreeTests:
    entity_created = (
        "Spawner entity created successfully",
        "Failed to create spawner entity"
    )
    spawner_type_set = (
        "Successfully set spawner type",
        "Failed to set spawner type"
    )
    empty_instance_count_validation = (
        "Expected number of empty instances planted",
        "Unexpected number of empty instances planted"
    )
    not_affected_by_allow_empty_assets = (
        "Instance count unaffected by Allow Empty Assets toggle",
        "Instance count was unexpectedly affected by Allow Empty Assets toggle"
    )


def EmptyInstanceSpawner_EmptySpawnerWorks():
    """
    Summary:
    Test aspects of the EmptyInstanceSpawner through the BehaviorContext and the Property Tree.

    :return: None
    """

    import azlmbr.legacy.general as general
    import azlmbr.math as math

    import editor_python_test_tools.hydra_editor_utils as hydra
    from largeworlds.large_worlds_utils import editor_dynveg_test_helper as dynveg
    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper as helper

    # 1) Open an existing simple level
    helper.init_idle()
    helper.open_level("Physics", "Base")
    general.set_current_view_position(512.0, 480.0, 38.0)

    # Grab the UUID that we need for creating an Empty Spawner
    empty_spawner_uuid = azlmbr.math.Uuid_CreateString('{23C40FD4-A55F-4BD3-BE5B-DC5423F217C2}', 0)

    # 2) Test EmptyInstanceSpawner BehaviorContext
    behavior_context_test_success = True
    empty_spawner = azlmbr.vegetation.EmptyInstanceSpawner()
    behavior_context_test_success = behavior_context_test_success and (empty_spawner is not None)
    behavior_context_test_success = behavior_context_test_success and (empty_spawner.typename == 'EmptyInstanceSpawner')
    Report.critical_result(BehaviorContextTests.spawner_initialized, behavior_context_test_success)
    Report.info(f'EmptyInstanceSpawner() BehaviorContext test: {behavior_context_test_success}')

    # 3) Test Descriptor BehaviorContext - setting spawnerType sets spawner too
    spawner_type_test_success = True
    descriptor = azlmbr.vegetation.Descriptor()
    spawner_type_test_success = spawner_type_test_success and hydra.get_set_property_test(descriptor, 'spawnerType', empty_spawner_uuid)
    spawner_type_test_success = spawner_type_test_success and (descriptor.spawner.typename == 'EmptyInstanceSpawner')
    Report.result(BehaviorContextTests.desc_spawnertype_sets_spawner, spawner_type_test_success)
    Report.info(f'Descriptor() BehaviorContext spawnerType test: {spawner_type_test_success}')

    # 4) Test Descriptor BehaviorContext - setting spawner sets spawnerType too
    spawner_test_success = True
    descriptor = azlmbr.vegetation.Descriptor()
    descriptor.spawner = empty_spawner
    spawner_test_success = spawner_test_success and (descriptor.spawnerType.Equal(empty_spawner_uuid))
    spawner_test_success = spawner_test_success and (descriptor.spawner.typename == 'EmptyInstanceSpawner')
    Report.result(BehaviorContextTests.desc_spawner_sets_spawnertype, spawner_test_success)
    Report.info(f'Descriptor() BehaviorContext spawner test: {spawner_test_success}')

    ### Setup for Property Tree set of tests

    # Create a new entity with required vegetation area components
    spawner_entity = hydra.Entity("Veg Area")
    spawner_entity.create_entity(
        math.Vector3(512.0, 512.0, 32.0),
        ["Vegetation Layer Spawner", "Box Shape", "Vegetation Asset List"]
        )
    Report.critical_result(PropertyTreeTests.entity_created, spawner_entity.id.IsValid())

    # Resize the Box Shape component
    new_box_dimensions = math.Vector3(16.0, 16.0, 16.0)
    box_dimensions_path = "Box Shape|Box Configuration|Dimensions"
    spawner_entity.get_set_test(1, box_dimensions_path, new_box_dimensions)

    # Create a surface to plant on
    dynveg.create_surface_entity("Surface Entity", math.Vector3(512.0, 512.0, 32.0), 1024.0, 1024.0, 1.0)

    # 5) Descriptor Property Tree test: spawner type can be set

    # - Validate the empty spawner type can be set correctly.
    property_tree_success = True
    property_tree_success = property_tree_success and spawner_entity.get_set_test(2, 'Configuration|Embedded Assets|[0]|Instance Spawner', empty_spawner_uuid)
    Report.result(PropertyTreeTests.spawner_type_set, property_tree_success)

    # This should result in 400 instances, since our box is 16 m x 16 m and by default the veg system plants
    # 20 instances per 16 meters
    num_expected_instances = 20 * 20
    property_tree_success = property_tree_success and helper.wait_for_condition(lambda: dynveg.validate_instance_count_in_entity_shape(spawner_entity.id, num_expected_instances), 5.0)
    Report.result(PropertyTreeTests.empty_instance_count_validation, property_tree_success)
    Report.info(f'Property Tree spawner type test: {property_tree_success}')

    # 6) Validate that the "Allow Empty Assets" setting doesn't affect the EmptyInstanceSpawner
    allow_empty_assets_success = True
    spawner_entity.get_set_test(0, 'Configuration|Allow Empty Assets', False)
    allow_empty_assets_success = allow_empty_assets_success and helper.wait_for_condition(lambda: dynveg.validate_instance_count_in_entity_shape(spawner_entity.id, num_expected_instances), 5.0)
    Report.result(PropertyTreeTests.not_affected_by_allow_empty_assets, allow_empty_assets_success)
    Report.info(f'Allow Empty Assets test: {allow_empty_assets_success}')


if __name__ == "__main__":

    from editor_python_test_tools.utils import Report
    Report.start_test(EmptyInstanceSpawner_EmptySpawnerWorks)
