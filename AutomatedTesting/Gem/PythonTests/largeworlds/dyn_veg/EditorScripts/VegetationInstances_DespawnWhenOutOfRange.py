"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
"""

"""
This script tests for regressions of "vegetation instances don't despawn correctly
when the camera moves beyond the range of all active vegetation areas".

This creates a new level and a vegetation area with 400 instances.
The expectation is that we will have 400 instances in that area when the camera is centered on it,
and 0 instances when the camera is moved sufficiently far away.
"""

import sys, os

import azlmbr.legacy.general as general
import azlmbr.math as math

sys.path.append(os.path.join(azlmbr.paths.devroot, 'AutomatedTesting', 'Gem', 'PythonTests'))
from automatedtesting_shared.editor_test_helper import EditorTestHelper
from largeworlds.large_worlds_utils import editor_dynveg_test_helper as dynveg


class TestVegetationInstances_DespawnWhenOutOfRange(EditorTestHelper):
    def __init__(self):
        EditorTestHelper.__init__(self, log_prefix='VegetationInstances_DespawnWhenOutOfRange', args=['level'])

    def run_test(self):

        # Create a new level
        self.test_success = self.create_level(
            self.get_arg('level'),
            heightmap_resolution=128,
            heightmap_meters_per_pixel=1,
            terrain_texture_resolution=128,
            use_terrain=False)

        # Create vegetation layer spawner
        world_center = math.Vector3(512.0, 512.0, 32.0)
        asset_path = os.path.join("Slices", "PurpleFlower.dynamicslice")
        spawner_entity = dynveg.create_vegetation_area("Spawner Instance", world_center, 16.0, 16.0, 16.0, asset_path)

        # Create a surface to spawn on
        dynveg.create_surface_entity("Spawner Entity", world_center, 16.0, 16.0, 1.0)

        # Get the root position of our veg area and use it to position our camera.
        # This is useful both to ensure that vegetation is spawned where we're querying and to
        # visually verify the number of instances in each box
        position = azlmbr.components.TransformBus(azlmbr.bus.Event, "GetWorldTranslation", spawner_entity.id)
        general.set_current_view_position(position.x, position.y, position.z + 30.0)
        general.set_current_view_rotation(-90.0, 0.0, 0.0)

        # When centered over the veg area, we expect to find 400 instances.
        # (16x16 area, 20 points per 16 meters)
        num_expected = 400
        result = self.wait_for_condition(lambda: dynveg.validate_instance_count_in_entity_shape(spawner_entity.id,
                                                                                                num_expected), 2.0)
        self.test_success = self.test_success and result

        # Move sufficiently far away from the veg area that it should all despawn.
        general.set_current_view_position(position.x - 1000.0, position.y - 1000.0, position.z + 30.0)

        # We now expect to find 0 instances.  If the bug exists, we will find 400 still.
        num_expected = 0
        result = self.wait_for_condition(lambda: dynveg.validate_instance_count_in_entity_shape(spawner_entity.id,
                                                                                                num_expected), 2.0)
        self.test_success = self.test_success and result


test = TestVegetationInstances_DespawnWhenOutOfRange()
test.run()
