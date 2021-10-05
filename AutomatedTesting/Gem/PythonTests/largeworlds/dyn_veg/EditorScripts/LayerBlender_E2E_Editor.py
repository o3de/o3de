"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


class Tests:
    level_created = (
        "Successfully created level",
        "Failed to create level"
    )
    blender_entity_created = (
        "Blender entity created successfully",
        "Failed to create Blender entity"
    )
    instance_count = (
        "Found the expected number of instances in the Blender area",
        "Found an unexpected number of instances in the Blender area"
    )
    instances_blended = (
        "Instances from each spawner are blended as expected",
        "Found an unexpected number of instances from each spawner"
    )
    saved_and_exported = (
        "Saved and exported level successfully",
        "Failed to save and export level"
    )


def LayerBlender_E2E_Editor():
    """
    Summary:
    A temporary level is loaded. Two vegetation areas with different meshes are added and then
    pinned to a vegetation blender. Screenshots are taken in the editor normal mode and in game mode.

    Expected Behavior:
    The specified assets plant in the specified blend area and are visible in the Viewport in
    Edit Mode, Game Mode.

    Test Steps:
     1) Create level
     2) Create 2 vegetation areas with different meshes
     3) Create Blender entity and pin the vegetation areas
     4) Take screenshot in normal mode
     5) Create a new entity with a Camera component for testing in the launcher
     6) Save level and take screenshot in game mode
     7) Export to engine

    Note:
    - This test file must be called from the Open 3D Engine Editor command terminal
    - Any passed and failed tests are written to the Editor.log file.
            Parsing the file or running a log_monitor are required to observe the test results.

    :return: None
    """

    import os
    from math import radians

    import azlmbr.asset as asset
    import azlmbr.areasystem as areasystem
    import azlmbr.legacy.general as general
    import azlmbr.paths as paths
    import azlmbr.bus as bus
    import azlmbr.components as components
    import azlmbr.math as math
    import azlmbr.entity as entity

    import editor_python_test_tools.hydra_editor_utils as hydra
    from largeworlds.large_worlds_utils import editor_dynveg_test_helper as dynveg
    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper as helper

    # 1) Create a new, temporary level
    lvl_name = "tmp_level"
    helper.init_idle()
    level_created = general.create_level_no_prompt(lvl_name, 1024, 1, 4096, False)
    general.idle_wait(1.0)
    Report.critical_result(Tests.level_created, level_created == 0)

    general.set_current_view_position(500.49, 498.69, 46.66)
    general.set_current_view_rotation(-42.05, 0.00, -36.33)

    # 2) Create 2 vegetation areas with different meshes
    purple_position = math.Vector3(504.0, 512.0, 32.0)
    purple_asset_path = os.path.join("Slices", "PurpleFlower.dynamicslice")
    spawner_entity_1 = dynveg.create_vegetation_area("Purple Spawner",
                                                     purple_position,
                                                     16.0, 16.0, 1.0,
                                                     purple_asset_path)

    pink_position = math.Vector3(520.0, 512.0, 32.0)
    pink_asset_path = os.path.join("Slices", "PinkFlower.dynamicslice")
    spawner_entity_2 = dynveg.create_vegetation_area("Pink Spawner",
                                                     pink_position,
                                                     16.0, 16.0, 1.0,
                                                     pink_asset_path)

    base_position = math.Vector3(512.0, 512.0, 32.0)
    dynveg.create_surface_entity("Surface Entity",
                                 base_position,
                                 16.0, 16.0, 1.0)

    hydra.add_level_component("Vegetation Debugger")

    # 3) Create Blender entity and pin the vegetation areas. We also add and attach a Lua script to validate in the
    # launcher for the follow-up test
    blender_entity = hydra.Entity("Blender")
    blender_entity.create_entity(
        base_position,
        ["Box Shape", "Vegetation Layer Blender", "Lua Script"]
    )
    Report.result(Tests.blender_entity_created, blender_entity.id.IsValid())

    blender_entity.get_set_test(0, "Box Shape|Box Configuration|Dimensions", math.Vector3(16.0, 16.0, 1.0))
    blender_entity.get_set_test(1, "Configuration|Vegetation Areas", [spawner_entity_1.id, spawner_entity_2.id])
    instance_counter_path = os.path.join("luascripts", "instance_counter_blender.lua")
    instance_counter_script = asset.AssetCatalogRequestBus(bus.Broadcast, "GetAssetIdByPath", instance_counter_path,
                                                           math.Uuid(), False)
    blender_entity.get_set_test(2, "Script properties|Asset", instance_counter_script)

    # 4) Verify instances in blender area are equally represented by both descriptors

    # Wait for instances to spawn
    general.run_console('veg_debugClearAllAreas')
    num_expected = 20 * 20
    success = helper.wait_for_condition(
        lambda: dynveg.validate_instance_count(base_position, 8.0, num_expected), 5.0)
    Report.critical_result(Tests.instance_count, success)

    box = math.Aabb_CreateCenterRadius(base_position, 8.0)
    instances = areasystem.AreaSystemRequestBus(bus.Broadcast, 'GetInstancesInAabb', box)
    pink_count = 0
    purple_count = 0
    for instance in instances:
        purple_asset_path = purple_asset_path.replace("\\", "/").lower()
        pink_asset_path = pink_asset_path.replace("\\", "/").lower()
        if instance.descriptor.spawner.GetSliceAssetPath() == pink_asset_path:
            pink_count += 1
        elif instance.descriptor.spawner.GetSliceAssetPath() == purple_asset_path:
            purple_count += 1
    Report.result(Tests.instances_blended, pink_count == purple_count and (pink_count + purple_count == num_expected))

    # 5) Move the default Camera entity for testing in the launcher
    cam_position = math.Vector3(500.0, 500.0, 47.0)
    cam_rot_degrees_vector = math.Vector3(radians(-55.0), radians(28.5), radians(-17.0))
    search_filter = entity.SearchFilter()
    search_filter.names = ["Camera"]
    search_entity_ids = entity.SearchBus(bus.Broadcast, 'SearchEntities', search_filter)
    components.TransformBus(bus.Event, "MoveEntity", search_entity_ids[0], cam_position)
    azlmbr.components.TransformBus(bus.Event, "SetLocalRotation", search_entity_ids[0], cam_rot_degrees_vector)

    # 6) Save and export to engine
    general.save_level()
    general.export_to_engine()
    pak_path = os.path.join(paths.products, "levels", lvl_name, "level.pak")
    Report.result(Tests.saved_and_exported, os.path.exists(pak_path))


if __name__ == "__main__":

    from editor_python_test_tools.utils import Report
    Report.start_test(LayerBlender_E2E_Editor)
