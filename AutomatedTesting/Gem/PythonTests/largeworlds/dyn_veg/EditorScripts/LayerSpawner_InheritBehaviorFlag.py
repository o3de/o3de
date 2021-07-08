"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import os
import sys

import azlmbr.math as math
import azlmbr.legacy.general as general
import azlmbr.paths
import azlmbr.surface_data as surface_data
import azlmbr.vegetation as vegetation

sys.path.append(os.path.join(azlmbr.paths.devroot, "AutomatedTesting", "Gem", "PythonTests"))
import editor_python_test_tools.hydra_editor_utils as hydra
from editor_python_test_tools.editor_test_helper import EditorTestHelper
from largeworlds.large_worlds_utils import editor_dynveg_test_helper as dynveg


class TestLayerSpawnerInheritBehavior(EditorTestHelper):
    def __init__(self):
        EditorTestHelper.__init__(self, log_prefix="LayerSpawner_InheritBehavior", args=["level"])

    def run_test(self):
        """
        Summary:
        C4762381 Verifies if Inherit Behavior Flag works as expected.

        Expected Result:
        The spawner with Inherit Behavior toggled off no longer obeys
        Vegetation Surface Mask Filter of the Vegetation Layer Blender entity and plants on the surface.

        :return: None
        """

        SURFACE_TAG = "test_tag"

        def set_dynamic_slice_asset(entity_obj, component_index, dynamic_slice_asset_path):
            dynamic_slice_spawner = vegetation.DynamicSliceInstanceSpawner()
            dynamic_slice_spawner.SetSliceAssetPath(dynamic_slice_asset_path)
            descriptor = hydra.get_component_property_value(
                entity_obj.components[component_index], "Configuration|Embedded Assets|[0]"
            )
            descriptor.spawner = dynamic_slice_spawner
            entity_obj.get_set_test(2, "Configuration|Embedded Assets|[0]", descriptor)

        # Create empty level
        self.test_success = self.create_level(
            self.args["level"],
            heightmap_resolution=1024,
            heightmap_meters_per_pixel=1,
            terrain_texture_resolution=4096,
            use_terrain=False,
        )

        general.set_current_view_position(512.0, 480.0, 38.0)

        # Create Emitter entity and add the required components
        position = math.Vector3(512.0, 512.0, 32.0)
        emitter_entity = dynveg.create_surface_entity("emitter_entity", position, 16.0, 16.0, 1.0)

        # Add surface tag to the Surface Tag Emitter
        tag = surface_data.SurfaceTag()
        tag.SetTag(SURFACE_TAG)
        pte = hydra.get_property_tree(emitter_entity.components[1])
        path = "Configuration|Generated Tags"
        pte.add_container_item(path, 0, tag)
        emitter_entity.get_set_test(1, "Configuration|Generated Tags|[0]", tag)

        # Create Blender entity and add required components
        components_to_add = ["Box Shape", "Vegetation Layer Blender"]
        blender_entity = hydra.Entity("blender_entity")
        blender_entity.create_entity(position, components_to_add)
        blender_entity.get_set_test(0, "Box Shape|Box Configuration|Dimensions", math.Vector3(16.0, 16.0, 1.0))

        # Create Vegetation area and assign a valid asset
        veg_1 = hydra.Entity("veg_1")
        veg_1.create_entity(
            position, ["Vegetation Layer Spawner", "Vegetation Reference Shape", "Vegetation Asset List"]
        )
        set_dynamic_slice_asset(veg_1, 2, os.path.join("Slices", "PinkFlower.dynamicslice"))
        veg_1.get_set_test(1, "Configuration|Shape Entity Id", blender_entity.id)

        # Create second vegetation area and assign a valid asset
        veg_2 = hydra.Entity("veg_2")
        veg_2.create_entity(
            position, ["Vegetation Layer Spawner", "Vegetation Reference Shape", "Vegetation Asset List"]
        )
        set_dynamic_slice_asset(veg_2, 2, os.path.join("Slices", "PurpleFlower.dynamicslice"))
        veg_2.get_set_test(1, "Configuration|Shape Entity Id", blender_entity.id)

        # Assign the vegetation areas to the Blender entity
        pte = hydra.get_property_tree(blender_entity.components[1])
        path = "Configuration|Vegetation Areas"
        pte.update_container_item(path, 0, veg_1.id)
        pte.add_container_item(path, 1, veg_2.id)

        # Add Vegetation Surface Mask Filter to the blender entity and add a Exclusion tag
        tag = surface_data.SurfaceTag()
        tag.SetTag(SURFACE_TAG)
        blender_entity.add_component("Vegetation Surface Mask Filter")
        pte = hydra.get_property_tree(blender_entity.components[2])
        path = "Configuration|Exclusion|Surface Tags"
        pte.add_container_item(path, 0, tag)
        blender_entity.get_set_test(2, "Configuration|Exclusion|Surface Tags|[0]", tag)

        # Toggle Inherit Behavior flag and verify vegetation instances
        self.log(
            f"Vegetation is not planted when Inherit Behavior flag is checked: {dynveg.validate_instance_count(position, 16.0, 0)}"
        )
        veg_1.get_set_test(0, "Configuration|Inherit Behavior", False)
        self.wait_for_condition(lambda: dynveg.validate_instance_count(position, 16.0, 400), 2.0)
        self.log(
            f"Vegetation plant when Inherit Behavior flag is unchecked: {dynveg.validate_instance_count(position, 16.0, 400)}"
        )


test = TestLayerSpawnerInheritBehavior()
test.run()
