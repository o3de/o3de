"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


class Tests:
    combined_instance_count_validation = (
        "Combined instance counts are as expected",
        "Found an unexpected number of instances"
    )
    replaced_asset_list_combined_instance_count_validation = (
        "Combined instance counts are as expected after replacing an Asset List reference",
        "Found an unexpected number of instances after replacing an Asset List reference"
    )
    removed_asset_lists_combined_instance_count_validation = (
        "Instance counts are as expected after removing the referenced Asset Lists",
        "Found an unexpected number of instances after removing the referenced Asset Lists"
    )


def AssetListCombiner_CombinedDescriptorsExpressInConfiguredArea():
    """
    Summary:
    Combined descriptors appear as expected in a vegetation area. Also verifies remove/replace of assigned Asset
    Lists.

    Expected Behavior:
    Vegetation fills in the area using the assets assigned to both Vegetation Asset Lists.

    Test Steps:
     1) Open a simple level
     2) Create 3 entities with Vegetation Asset List components set to spawn different descriptors
     3) Create a planting surface and add a Vegetation System Settings level component with instances set to spawn
        on center instead of corner
     4) Create a spawner using a Vegetation Asset List Combiner component and a Weight Selector, and disallow
        spawning empty assets
     5) Add 2 of the Asset List entities to the Vegetation Asset List Combiner component (PinkFlower and Empty)
     6) Create a Constant Gradient entity as a child of the spawner entity, and a Dither Gradient Modifier entity
        as a child of the Constant Gradient entity, and configure for a checkerboard pattern
     7) Pin the Dither Gradient Entity to the Asset Weight Selector of the spawner entity
     8) Validate instance count with configured Asset List Combiner
     9) Replace the reference to the 2nd asset list on the Vegetation Asset List Combiner component and validate
        instance count
    10) Remove the referenced Asset Lists on the Asset List Combiner, Disable/Re-enable the Asset List
        Combiner component to force a refresh, and validate instance count

    Note:
    - This test file must be called from the Open 3D Engine Editor command terminal
    - Any passed and failed tests are written to the Editor.log file.
            Parsing the file or running a log_monitor are required to observe the test results.

    :return: None
    """

    import os
    from pathlib import Path

    import azlmbr.bus as bus
    import azlmbr.editor as editor
    import azlmbr.legacy.general as general
    import azlmbr.math as math
    import azlmbr.vegetation as vegetation
    import azlmbr.prefab as prefab

    import editor_python_test_tools.hydra_editor_utils as hydra
    from largeworlds.large_worlds_utils import editor_dynveg_test_helper as dynveg
    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper as helper

    def create_asset_list_entity(name, center, target_prefab):
        asset_list_entity = hydra.Entity(name)
        asset_list_entity.create_entity(
            center,
            ["Vegetation Asset List"]
        )
        if asset_list_entity.id.IsValid():
            print(f"'{asset_list_entity.name}' created")

        if target_prefab:
            # Get the in-memory spawnable asset id if exists
            spawnable_name = Path(target_prefab.file_path).stem
            spawnable_asset_id = prefab.PrefabPublicRequestBus(bus.Broadcast, 'GetInMemorySpawnableAssetId',
                                                               spawnable_name)

            # Create the in-memory spawnable asset from given prefab if the spawnable does not exist
            if not spawnable_asset_id.is_valid():
                create_spawnable_result = prefab.PrefabPublicRequestBus(bus.Broadcast, 'CreateInMemorySpawnableAsset',
                                                                        target_prefab.file_path,
                                                                        spawnable_name)
                assert create_spawnable_result.IsSuccess(), \
                    f"Prefab operation 'CreateInMemorySpawnableAssets' failed. Error: {create_spawnable_result.GetError()}"
                spawnable_asset_id = create_spawnable_result.GetValue()
        else:
            spawnable_asset_id = None

        # Set the vegetation area to a prefab instance spawner with a specific prefab asset selected
        descriptor = hydra.get_component_property_value(asset_list_entity.components[0],
                                                        'Configuration|Embedded Assets|[0]')
        prefab_spawner = vegetation.PrefabInstanceSpawner()
        prefab_spawner.SetPrefabAssetId(spawnable_asset_id)
        descriptor.spawner = prefab_spawner
        asset_list_entity.get_set_test(0, "Configuration|Embedded Assets|[0]", descriptor)
        return asset_list_entity

    # 1) Open an existing simple level
    hydra.open_base_level()

    # Set view of planting area for visual debugging
    general.set_current_view_position(512.0, 500.0, 38.0)
    general.set_current_view_rotation(-20.0, 0.0, 0.0)

    # 2) Create 3 entities with Vegetation Asset List components set to spawn different descriptors
    center_point = math.Vector3(512.0, 512.0, 32.0)
    pink_flower_asset_path = os.path.join("assets", "objects", "foliage", "grass_flower_pink.fbx.azmodel")
    pink_flower_prefab = dynveg.create_temp_mesh_prefab(pink_flower_asset_path, "AssetList_PinkFlower")[0]
    purple_flower_asset_path = os.path.join("assets", "objects", "foliage", "grass_flower_pink.fbx.azmodel")
    purple_flower_prefab = dynveg.create_temp_mesh_prefab(purple_flower_asset_path, "AssetList_PurpleFlower")[0]
    asset_list_entity = create_asset_list_entity("Asset List 1", center_point, pink_flower_prefab)
    asset_list_entity2 = create_asset_list_entity("Asset List 2", center_point, None)
    asset_list_entity3 = create_asset_list_entity("Asset List 3", center_point, purple_flower_prefab)

    # 3) Create a planting surface and add a Vegetation System Settings level component with instances set to spawn
    # on center instead of corner
    dynveg.create_surface_entity("Surface Entity", center_point, 32.0, 32.0, 1.0)
    veg_system_settings_component = hydra.add_level_component("Vegetation System Settings")
    editor.EditorComponentAPIBus(bus.Broadcast, "SetComponentProperty", veg_system_settings_component,
                                 'Configuration|Area System Settings|Sector Point Snap Mode', 1)

    # 4) Create a spawner using a Vegetation Asset List Combiner component and a Weight Selector, and disallow
    # spawning empty assets
    spawner_entity = dynveg.create_prefab_vegetation_area("Spawner Entity", center_point, 16.0, 16.0, 16.0, None)
    spawner_entity.remove_component("Vegetation Asset List")
    spawner_entity.add_component("Vegetation Asset List Combiner")
    spawner_entity.add_component("Vegetation Asset Weight Selector")
    spawner_entity.get_set_test(0, "Configuration|Allow Empty Assets", False)

    # 5) Add the Asset List entities to the Vegetation Asset List Combiner component
    asset_list_entities = [asset_list_entity.id, asset_list_entity2.id]
    spawner_entity.get_set_test(2, "Configuration|Descriptor Providers", asset_list_entities)

    # 6) Create a Constant Gradient entity as a child of the spawner entity, and a Dither Gradient Modifier entity
    # as a child of the Constant Gradient entity, and configure for a checkerboard pattern
    components_to_add = ["Constant Gradient"]
    constant_gradient_entity = hydra.Entity("Constant Gradient Entity")
    constant_gradient_entity.create_entity(center_point, components_to_add, parent_id=spawner_entity.id)
    constant_gradient_entity.get_set_test(0, "Configuration|Value", 0.5)

    components_to_add = ["Dither Gradient Modifier"]
    dither_gradient_entity = hydra.Entity("Dither Gradient Entity")
    dither_gradient_entity.create_entity(center_point, components_to_add, parent_id=constant_gradient_entity.id)
    dither_gradient_entity.get_set_test(0, "Configuration|Gradient|Gradient Entity Id", constant_gradient_entity.id)

    # 7) Pin the Dither Gradient Entity to the Asset Weight Selector of the spawner entity
    spawner_entity.get_set_test(3, "Configuration|Gradient|Gradient Entity Id", dither_gradient_entity.id)

    # 8) Validate instance count. We should now have 200 instances in the spawner area as every other instance
    # should be an empty asset which the spawner is set to disallow
    num_expected = 20 * 20 / 2
    success = helper.wait_for_condition(lambda: dynveg.validate_instance_count_in_entity_shape(spawner_entity.id,
                                                                                             num_expected), 5.0)
    Report.result(Tests.combined_instance_count_validation, success)

    # 9) Replace the reference to the 2nd asset list on the Vegetation Asset List Combiner component and validate
    # instance count. Should now be 400 instances as the empty spaces can now be claimed by the new descriptor
    spawner_entity.get_set_test(2, "Configuration|Descriptor Providers|[1]", asset_list_entity3.id)
    num_expected = 20 * 20
    success = helper.wait_for_condition(lambda: dynveg.validate_instance_count_in_entity_shape(spawner_entity.id,
                                                                                             num_expected), 5.0)
    Report.result(Tests.replaced_asset_list_combined_instance_count_validation, success)

    # 10) Remove the referenced Asset Lists on the Asset List Combiner, Disable/Re-enable the Asset List
    # Combiner component to force a refresh, and validate instance count. We should now have 0 instances.
    pte = hydra.get_property_tree(spawner_entity.components[2])
    path = "Configuration|Descriptor Providers"
    pte.reset_container(path)
    # Component refresh is currently necessary due to container operations not causing a refresh (LY-120947)
    editor.EditorComponentAPIBus(bus.Broadcast, "DisableComponents", [spawner_entity.components[2]])
    editor.EditorComponentAPIBus(bus.Broadcast, "EnableComponents", [spawner_entity.components[2]])
    num_expected = 0
    success = helper.wait_for_condition(lambda: dynveg.validate_instance_count_in_entity_shape(spawner_entity.id,
                                                                                             num_expected), 5.0)
    Report.result(Tests.removed_asset_lists_combined_instance_count_validation, success)


if __name__ == "__main__":

    from editor_python_test_tools.utils import Report
    Report.start_test(AssetListCombiner_CombinedDescriptorsExpressInConfiguredArea)
