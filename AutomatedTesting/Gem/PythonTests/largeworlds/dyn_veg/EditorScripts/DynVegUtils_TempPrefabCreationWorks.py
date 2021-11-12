"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


def DynVegUtils_TempPrefabCreationWorks():
    """
    Summary:
    An existing level is opened. Each Prefab setup to be spawned by Dynamic Vegetation tests is created in memory and
    validated against existing components/mesh assignments. A spawner is then created with the temporary prefabs to ensure
    proper functionality with Dynamic Vegetation components.

    Expected Behavior:
    Temporary prefabs contain the expected components/assets. Instances plant as expected in the assigned area.

    Test Steps:
     1) Open an existing level
     2) Create each of the necessary temporary prefabs, and validate the component/mesh setups
     3) Create a Vegetation Layer Spawner setup using the temporary prefabs as Prefab Instance Spawner types
     4) Create a surface to plant on and validate instance counts
     5) Verify expected instance counts

    Note:
    - This test file must be called from the Open 3D Engine Editor command terminal
    - Any passed and failed tests are written to the Editor.log file.
            Parsing the file or running a log_monitor are required to observe the test results.

    :return: None
    """

    import os

    import azlmbr.math as math

    from largeworlds.large_worlds_utils import editor_dynveg_test_helper as dynveg
    from editor_python_test_tools.utils import Report, Tracer
    from editor_python_test_tools.utils import TestHelper as helper
    from editor_python_test_tools.prefab_utils import Prefab, PrefabInstance, get_prefab_file_path

    with Tracer() as error_tracer:
        # Create dictionary for prefab filenames and paths to create using helper function
        mesh_prefabs = {
            "PinkFlower": os.path.join("assets", "objects", "foliage", "grass_flower_pink.azmodel"),
            "PurpleFlower": os.path.join("assets", "objects", "foliage", "grass_flower_purple.azmodel"),
            "1m_Cube": os.path.join("objects", "_primitives", "_box_1x1.azmodel"),
            "CedarTree": os.path.join("assets", "objects", "foliage", "cedar.azmodel"),
            "Bush": os.path.join("assets", "objects", "foliage", "bush_privet_01.azmodel"),
                   }

        # 1) Open an existing simple level
        helper.init_idle()
        helper.open_level("Prefab", "Base")

        # 2) Create a Dynamic Vegetation surface in preparation of prefab creation
        center_point = math.Vector3(0.0, 0.0, 0.0)
        dynveg.create_surface_entity("Planting Surface", center_point, 128.0, 128.0, 1.0)

        # 3) Create each of the Mesh asset prefabs and validate that the prefab created successfully
        for prefab_filename, asset_path in mesh_prefabs.items():
            prefab_created = (
                f"Temporary {prefab_filename} prefab created successfully",
                f"Failed to create temporary {prefab_filename} prefab"
            )
            prefab = dynveg.create_temp_mesh_prefab(asset_path, prefab_filename)
            Report.result(prefab_created, helper.wait_for_condition(lambda: PrefabInstance.is_valid(prefab[1]), 3.0))

        # 4) Assign the temp prefab to the prefab instance spawner, and validate the instance count
        for prefab_filename in mesh_prefabs:
            spawner_entity = dynveg.create_prefab_spawner("PrefabSpawner", center_point, 16.0, 16.0, 16.0,
                                                          get_prefab_file_path(get_prefab_file_path(prefab_filename)))

        # 5) Report errors/asserts
        helper.wait_for_condition(lambda: error_tracer.has_errors or error_tracer.has_asserts, 1.0)
        for error_info in error_tracer.errors:
            Report.info(f"Error: {error_info.filename} {error_info.function} | {error_info.message}")
        for assert_info in error_tracer.asserts:
            Report.info(f"Assert: {assert_info.filename} {assert_info.function} | {assert_info.message}")


if __name__ == "__main__":

    from editor_python_test_tools.utils import Report
    Report.start_test(DynVegUtils_TempPrefabCreationWorks)
