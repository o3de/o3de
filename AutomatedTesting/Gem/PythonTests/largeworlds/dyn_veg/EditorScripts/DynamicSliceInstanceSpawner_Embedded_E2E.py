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
    spawner_entity_created = (
        "Spawner entity created successfully",
        "Failed to create spawner entity"
    )
    surface_entity_created = (
        "Surface entity created successfully",
        "Failed to create surface entity"
    )
    instance_count = (
        "Found the expected number of instances",
        "Found an unexpected number of instances"
    )
    saved_and_exported = (
        "Saved and exported level successfully",
        "Failed to save and export level"
    )


def DynamicSliceInstanceSpawner_Embedded_E2E():
    """
    Summary:
    A new temporary level is created. Surface for planting is created. Simple vegetation area is created using
    Dynamic Slice Instance Spawner type.

    Expected Behavior:
    Instances plant as expected in the assigned area.

    Test Steps:
     1) Create level
     2) Create a Vegetation Layer Spawner setup using Dynamic Slice Instance Spawner type assets
     3) Create a surface to plant on
     4) Verify expected instance counts
     5) Add a camera component looking at the planting area for visual debugging
     6) Save and export to engine

    Note:
    - This test file must be called from the Open 3D Engine Editor command terminal
    - Any passed and failed tests are written to the Editor.log file.
            Parsing the file or running a log_monitor are required to observe the test results.

    :return: None
    """

    import os

    import azlmbr.asset as asset
    import azlmbr.legacy.general as general
    import azlmbr.bus as bus
    import azlmbr.components as components
    import azlmbr.entity as entity
    import azlmbr.math as math
    import azlmbr.paths as paths

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
    general.set_current_view_position(512.0, 480.0, 38.0)

    # 2) Create a new entity with required vegetation area components and Script Canvas component for launcher test
    center_point = math.Vector3(512.0, 512.0, 32.0)
    asset_path = os.path.join("Slices", "PinkFlower.dynamicslice")
    spawner_entity = dynveg.create_vegetation_area("Instance Spawner", center_point, 16.0, 16.0, 1.0, asset_path)
    spawner_entity.add_component("Script Canvas")
    instance_counter_path = os.path.join("scriptcanvas", "instance_counter.scriptcanvas")
    instance_counter_script = asset.AssetCatalogRequestBus(bus.Broadcast, "GetAssetIdByPath", instance_counter_path,
                                                           math.Uuid(), False)
    spawner_entity.get_set_test(3, "Script Canvas Asset|Script Canvas Asset", instance_counter_script)
    Report.result(Tests.spawner_entity_created, spawner_entity.id.IsValid() and hydra.has_components(spawner_entity.id,
                                                                                                     ["Script Canvas"]))

    # 3) Create a surface to plant on
    surface_entity = dynveg.create_surface_entity("Planting Surface", center_point, 128.0, 128.0, 1.0)
    Report.result(Tests.surface_entity_created, surface_entity.id.IsValid())

    # 4) Verify instance counts are accurate
    num_expected_instances = 20 * 20
    success = helper.wait_for_condition(lambda: dynveg.validate_instance_count_in_entity_shape(spawner_entity.id,
                                                                                               num_expected_instances), 5.0)
    Report.result(Tests.instance_count, success)

    # 5) Move the default Camera entity for testing in the launcher
    cam_position = math.Vector3(512.0, 500.0, 35.0)
    search_filter = entity.SearchFilter()
    search_filter.names = ["Camera"]
    search_entity_ids = entity.SearchBus(bus.Broadcast, 'SearchEntities', search_filter)
    components.TransformBus(bus.Event, "MoveEntity", search_entity_ids[0], cam_position)

    # 6) Save and export to engine
    general.save_level()
    general.export_to_engine()
    pak_path = os.path.join(paths.products, "levels", lvl_name, "level.pak")
    Report.result(Tests.saved_and_exported, os.path.exists(pak_path))


if __name__ == "__main__":

    from editor_python_test_tools.utils import Report
    Report.start_test(DynamicSliceInstanceSpawner_Embedded_E2E)
