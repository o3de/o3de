"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.


C1519572: Create a dynamic slice
https://testrail.agscollab.com/index.php?/cases/view/1519572
"""

import os
import sys

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import azlmbr.legacy.general as general
import azlmbr.bus as bus
import azlmbr.editor as editor
import azlmbr.entity as EntityId
import azlmbr.asset as asset
import azlmbr.math as math
import azlmbr.slice as slice
from Tests.editor.editor_utils.editor_test_helper import EditorTestHelper


class CreateDynamicSliceTest(EditorTestHelper):
    def __init__(self):
        EditorTestHelper.__init__(self, log_prefix="create_dynamic_slice", args=["level"])

    def run_test(self):
        """
        Summary:
        Create a dynamic slice

        Expected Behavior:
        Dynamic slices can be set/unset

        Test Steps:
        1) Open level
        2) Create new entity
        3) Create new slice from the newly created entity
        4) Set an existing slice to dynamic
        5) Unset the dynamic slice

        Note:
        - This test file must be called from the Lumberyard Editor command terminal
        - Any passed and failed tests are written to the Editor.log file.
                Parsing the file or running a log_monitor are required to observe the test results.

        :return: None
        """

        def is_slice_dynamic(asset_id):
            if slice.SliceRequestBus(bus.Broadcast, "IsSliceDynamic", asset_id):
                return "Dynamic"
            return "Not Dynamic"

        def path_is_valid_asset(asset_path):
            asset_id = asset.AssetCatalogRequestBus(bus.Broadcast, "GetAssetIdByPath", asset_path, math.Uuid(), False)
            return asset_id.invoke("IsValid")

        # 1) Open level
        self.test_success = self.create_level(
            self.args["level"],
            heightmap_resolution=1024,
            heightmap_meters_per_pixel=1,
            terrain_texture_resolution=4096,
            use_terrain=True,
        )
        general.idle_wait(3.0)

        # 2) Create new entity
        entity_position = math.Vector3(125.0, 136.0, 32.0)
        entity_id = editor.ToolsApplicationRequestBus(
            bus.Broadcast, "CreateNewEntityAtPosition", entity_position, EntityId.EntityId()
        )

        # 3) Create new slice from the newly created entity
        slice_path = os.path.join("slices", "TestSlice.slice")
        slice.SliceRequestBus(bus.Broadcast, "CreateNewSlice", entity_id, slice_path)
        self.wait_for_condition(lambda: path_is_valid_asset(slice_path), 10.0)

        # 4) Set an existing slice to dynamic
        asset_id = asset.AssetCatalogRequestBus(bus.Broadcast, "GetAssetIdByPath", slice_path, math.Uuid(), False)
        print(f"Slice Status (Before setting to dynamic): {is_slice_dynamic(asset_id)}")
        slice.SliceRequestBus(bus.Broadcast, "SetSliceDynamic", asset_id, True)
        self.wait_for_condition(lambda: slice.SliceRequestBus(bus.Broadcast, "IsSliceDynamic", asset_id), 5.0)
        print(f"Slice Status (After setting to dynamic): {is_slice_dynamic(asset_id)}")

        # 5) Unset the dynamic slice
        print(f"Slice Status (Before unsetting to dynamic): {is_slice_dynamic(asset_id)}")
        slice.SliceRequestBus(bus.Broadcast, "SetSliceDynamic", asset_id, False)
        self.wait_for_condition(lambda: not slice.SliceRequestBus(bus.Broadcast, "IsSliceDynamic", asset_id), 5.0)
        print(f"Slice Status (After unsetting to dynamic): {is_slice_dynamic(asset_id)}")


test = CreateDynamicSliceTest()
test.run()
