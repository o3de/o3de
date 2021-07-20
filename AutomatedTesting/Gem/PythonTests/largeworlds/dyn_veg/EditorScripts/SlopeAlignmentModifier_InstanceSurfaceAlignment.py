"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import os
from math import radians
import sys

import azlmbr.areasystem as areasystem
import azlmbr.asset as asset
import azlmbr.bus as bus
import azlmbr.components as components
import azlmbr.editor as editor
import azlmbr.legacy.general as general
import azlmbr.math as math
import azlmbr.paths

sys.path.append(os.path.join(azlmbr.paths.devroot, 'AutomatedTesting', 'Gem', 'PythonTests'))
import editor_python_test_tools.hydra_editor_utils as hydra
from editor_python_test_tools.editor_test_helper import EditorTestHelper
from largeworlds.large_worlds_utils import editor_dynveg_test_helper as dynveg


class TestSlopeAlignmentModifier(EditorTestHelper):
    def __init__(self):
        EditorTestHelper.__init__(self, log_prefix="SlopeAlignmentModifier", args=["level"])

    def run_test(self):
        """
        Summary:
        C4896941 Verifies instances properly align to surfaces based on configuration of the Vegetation Slope Alignment
        Modifier.

        :return: None
        """

        def verify_proper_alignment(instance, rot_degrees_vec):
            expected_rotation = math.Quaternion()
            expected_rotation.SetFromEulerDegrees(rot_degrees_vec)
            if instance.alignment.IsClose(expected_rotation):
                return True
            self.log(f"Expected rotation of {expected_rotation}, Found {instance.alignment}")
            return False

        # Create empty level
        self.test_success = self.create_level(
            self.args["level"],
            heightmap_resolution=1024,
            heightmap_meters_per_pixel=1,
            terrain_texture_resolution=4096,
            use_terrain=False,
        )

        general.set_current_view_position(512.0, 480.0, 38.0)

        # Create a spawner entity setup with all needed components
        center_point = math.Vector3(512.0, 512.0, 32.0)
        asset_path = os.path.join("Slices", "PinkFlower.dynamicslice")
        spawner_entity = dynveg.create_vegetation_area("Instance Spawner", center_point, 16.0, 16.0, 32.0, asset_path)

        # Create a sloped mesh surface for the instances to plant on
        mesh_asset_path = os.path.join("objects", "_primitives", "_box_1x1.azmodel")
        mesh_asset = asset.AssetCatalogRequestBus(bus.Broadcast, "GetAssetIdByPath", mesh_asset_path, math.Uuid(),
                                                  False)
        rotation = math.Vector3(0.0, radians(45.0), 0.0)
        scale = math.Vector3(10.0, 10.0, 10.0)
        surface_entity = hydra.Entity("Surface Entity")
        surface_entity.create_entity(
            center_point,
            ["Mesh", "Mesh Surface Tag Emitter"]
        )
        if surface_entity.id.IsValid():
            print(f"'{surface_entity.name}' created")
        hydra.get_set_test(surface_entity, 0, "Controller|Configuration|Mesh Asset", mesh_asset)
        components.TransformBus(bus.Event, "SetLocalRotation", surface_entity.id, rotation)
        components.TransformBus(bus.Event, "SetLocalScale", surface_entity.id, scale)

        # Add a Vegetation Debugger component to allow refreshing instances
        hydra.add_level_component("Vegetation Debugger")

        # Add Vegetation Slope Alignment Modifier to the spawner entity
        spawner_entity.add_component("Vegetation Slope Alignment Modifier")

        # Set Alignment Coefficient Min/Max to 1 on the Slope Alignment Modifier
        spawner_entity.get_set_test(3, "Configuration|Alignment Coefficient Min", 1.0)
        spawner_entity.get_set_test(3, "Configuration|Alignment Coefficient Max", 1.0)

        # Create new child entity with a Constant Gradient
        child_vegetation_id = editor.ToolsApplicationRequestBus(bus.Broadcast, "CreateNewEntity", spawner_entity.id)
        child_vegetation = hydra.Entity("Child Vegetation Entity", child_vegetation_id)
        components_to_add = ["Constant Gradient"]
        child_vegetation.components = []
        for component in components_to_add:
            child_vegetation.components.append(hydra.add_component(component, child_vegetation_id))

        # Reference the Constant Gradient on the Slope Alignment Modifier component
        spawner_entity.get_set_test(3, "Configuration|Gradient|Gradient Entity Id", child_vegetation_id)

        # Verify instances are have planted and are aligned to slope as expected
        num_expected = 20 * 20
        self.test_success = self.test_success and self.wait_for_condition(
            lambda: dynveg.validate_instance_count_in_entity_shape(spawner_entity.id, num_expected), 5.0)

        box = azlmbr.shape.ShapeComponentRequestsBus(bus.Event, 'GetEncompassingAabb', spawner_entity.id)
        instances = areasystem.AreaSystemRequestBus(bus.Broadcast, 'GetInstancesInAabb', box)

        if self.test_success and num_expected == len(instances):
            for instance in instances:
                self.test_success = verify_proper_alignment(instance, math.Vector3(0.0, 45.0, 0.0)) and \
                                    self.test_success

        # Change Min/Max to 0.0 and verify proper alignment
        spawner_entity.get_set_test(3, "Configuration|Alignment Coefficient Min", 0.0)
        spawner_entity.get_set_test(3, "Configuration|Alignment Coefficient Max", 0.0)
        general.run_console('veg_debugClearAllAreas')
        self.test_success = self.test_success and self.wait_for_condition(
            lambda: dynveg.validate_instance_count_in_entity_shape(spawner_entity.id, num_expected), 5.0)

        box = azlmbr.shape.ShapeComponentRequestsBus(bus.Event, 'GetEncompassingAabb', spawner_entity.id)
        instances = areasystem.AreaSystemRequestBus(bus.Broadcast, 'GetInstancesInAabb', box)

        if self.test_success and num_expected == len(instances):
            for instance in instances:
                self.test_success = verify_proper_alignment(instance, math.Vector3(0.0, 0.0, 0.0)) and self.test_success


test = TestSlopeAlignmentModifier()
test.run()
