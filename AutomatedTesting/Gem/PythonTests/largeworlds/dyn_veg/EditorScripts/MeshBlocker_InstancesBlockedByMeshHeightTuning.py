"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
"""

import os
import sys

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import azlmbr
import azlmbr.asset as asset
import azlmbr.bus as bus
import azlmbr.components as components
import azlmbr.math as math


sys.path.append(os.path.join(azlmbr.paths.devroot, 'AutomatedTesting', 'Gem', 'PythonTests'))
import automatedtesting_shared.hydra_editor_utils as hydra
from automatedtesting_shared.editor_test_helper import EditorTestHelper
from largeworlds.large_worlds_utils import editor_dynveg_test_helper as dynveg


class test_MeshBlocker_InstancesBlockedByMeshHeightTuning(EditorTestHelper):
    def __init__(self):
        EditorTestHelper.__init__(self, log_prefix="MeshBlocker_InstancesBlockedByMeshHeightTuning", args=["level"])

    def run_test(self):
        """
        Summary:
        A temporary level is created, then a simple vegetation area is created. A blocker area is created and it is
        verified that the tuning of the height percent blocker setting works as expected.

        Expected Behavior:
        Vegetation is blocked only around the trunk of the tree, while it still plants under the areas covered by branches.

        Test Steps:
         1) Create level
         2) Create entity with components "Vegetation Layer Spawner", "Vegetation Asset List", "Box Shape"
         3) Create surface entity
         4) Create blocker entity with sphere mesh
         5) Adjust the height Min/Max percentage values of blocker
         6) Verify spawned instance counts are accurate after adjusting height Max percentage of Blocker Entity

        Note:
        - Any passed and failed tests are written to the Editor.log file.
                Parsing the file or running a log_monitor are required to observe the test results.

        :return: None
        """

        # 1) Create level
        self.test_success = self.create_level(
            self.args["level"],
            heightmap_resolution=1024,
            heightmap_meters_per_pixel=1,
            terrain_texture_resolution=4096,
            use_terrain=False,
        )

        # 2) Create entity with components "Vegetation Layer Spawner", "Vegetation Asset List", "Box Shape"
        entity_position = math.Vector3(512.0, 512.0, 32.0)
        asset_path = os.path.join("Slices", "PurpleFlower.dynamicslice")
        spawner_entity = dynveg.create_vegetation_area("Instance Spawner",
                                                       entity_position,
                                                       10.0, 10.0, 10.0,
                                                       asset_path)

        # 3) Create surface entity to plant on
        dynveg.create_surface_entity("Surface Entity", entity_position, 10.0, 10.0, 1.0)

        # 4) Create blocker entity with cube mesh
        entity_position = math.Vector3(512.0, 512.0, 36.0)
        blocker_entity = hydra.Entity("Blocker Entity")
        blocker_entity.create_entity(entity_position,
                                     ["Mesh", "Vegetation Layer Blocker (Mesh)"])
        if blocker_entity.id.IsValid():
            print(f"'{blocker_entity.name}' created")

        sphereId = asset.AssetCatalogRequestBus(
            bus.Broadcast, "GetAssetIdByPath", os.path.join("objects", "default", "primitive_sphere.cgf"), math.Uuid(),
            False)
        blocker_entity.get_set_test(0, "MeshComponentRenderNode|Mesh asset", sphereId)
        components.TransformBus(bus.Event, "SetLocalScale", blocker_entity.id, math.Vector3(5.0, 5.0, 5.0))

        # 5) Adjust the height Max percentage values of blocker
        blocker_entity.get_set_test(1, "Configuration|Mesh Height Percent Max", 0.8)

        # 6) Verify spawned instance counts are accurate after adjusting height Max percentage of Blocker Entity
        # The number of "PurpleFlower" instances that plant on a 10 x 10 surface minus those blocked by the sphere at
        # 80% max height factored in.
        num_expected = 117
        result = self.wait_for_condition(lambda: dynveg.validate_instance_count_in_entity_shape(spawner_entity.id,
                                                                                                num_expected), 2.0)
        self.test_success = self.test_success and result


test = test_MeshBlocker_InstancesBlockedByMeshHeightTuning()
test.run()
