"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


def DynVegUtils_TempPrefabCreationWorks():
    """
    Summary:
    An existing level is opened. Each Prefab setup to be spawned by Dynamic Vegetation tests is created in memory and
    validated against existing test components/mesh assignments.

    Expected Behavior:
    Temporary prefabs contain the expected components/assets.

    Test Steps:
     1) Open an existing level
     2) Create each of the necessary temporary Mesh prefabs, and validate the component/mesh setups
     3) Create the necessary temporary PhysX Collider, and validate the component setup
     4) Report errors/asserts

    :return: None
    """

    import os

    import azlmbr.asset as asset
    import azlmbr.bus as bus
    import azlmbr.math as math

    from Prefab.tests import PrefabTestUtils as prefab_test_utils
    from largeworlds.large_worlds_utils import editor_dynveg_test_helper as dynveg
    from editor_python_test_tools.utils import Report, Tracer
    from editor_python_test_tools.utils import TestHelper as helper
    from editor_python_test_tools.prefab_utils import PrefabInstance

    with Tracer() as error_tracer:
        # Create dictionary for prefab filenames and paths to create using helper function
        mesh_prefabs = {
            "UtilsTest_PinkFlower": os.path.join("assets", "objects", "foliage", "grass_flower_pink.fbx.azmodel"),
            "UtilsTest_PurpleFlower": os.path.join("assets", "objects", "foliage", "grass_flower_purple.fbx.azmodel"),
            "UtilsTest_1m_Cube": os.path.join("objects", "_primitives", "_box_1x1.fbx.azmodel"),
            "UtilsTest_CedarTree": os.path.join("assets", "objects", "foliage", "cedar.fbx.azmodel"),
            "UtilsTest_Bush": os.path.join("assets", "objects", "foliage", "bush_privet_01.fbx.azmodel"),
                   }

        # 1) Open an existing simple level
        prefab_test_utils.open_base_tests_level()

        # 2) Create each of the Mesh asset prefabs and validate that the prefab created successfully
        for prefab_filename, asset_path in mesh_prefabs.items():
            mesh_prefab_created = (
                f"Temporary mesh prefab: {prefab_filename} created successfully",
                f"Failed to create temporary mesh prefab: {prefab_filename}"
            )
            prefab = dynveg.create_temp_mesh_prefab(asset_path, prefab_filename)
            Report.result(mesh_prefab_created, helper.wait_for_condition(lambda:
                                                                         PrefabInstance.is_valid(prefab[1]), 3.0))

        # 3) Create temp PhysX Collider prefab and validate that the prefab created successfully
        physx_prefab_filename = "CedarTree_Collision"
        physx_collider_prefab_created = (
            f"Temporary mesh prefab: {physx_prefab_filename} created successfully",
            f"Failed to create temporary mesh prefab: {physx_prefab_filename}"
        )
        test_physx_mesh_asset_id = asset.AssetCatalogRequestBus(bus.Broadcast, "GetAssetIdByPath", os.path.join(
            "assets", "objects", "foliage", "cedar.fbx.pxmesh"), math.Uuid(), False)
        dynveg.create_temp_physx_mesh_collider(test_physx_mesh_asset_id, physx_prefab_filename)
        Report.result(physx_collider_prefab_created, helper.wait_for_condition(lambda:
                                                                               PrefabInstance.is_valid(prefab[1]), 3.0))

        # 4) Report errors/asserts
        helper.wait_for_condition(lambda: error_tracer.has_errors or error_tracer.has_asserts, 1.0)
        for error_info in error_tracer.errors:
            Report.info(f"Error: {error_info.filename} {error_info.function} | {error_info.message}")
        for assert_info in error_tracer.asserts:
            Report.info(f"Assert: {assert_info.filename} {assert_info.function} | {assert_info.message}")


if __name__ == "__main__":

    from editor_python_test_tools.utils import Report
    Report.start_test(DynVegUtils_TempPrefabCreationWorks)
