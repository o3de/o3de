"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import os
import sys

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import azlmbr
import azlmbr.legacy.general as general
import azlmbr.math as math

sys.path.append(os.path.join(azlmbr.paths.devroot, 'AutomatedTesting', 'Gem', 'PythonTests'))
from automatedtesting_shared.editor_test_helper import EditorTestHelper
from largeworlds.large_worlds_utils import editor_dynveg_test_helper as dynveg


class TestLayerSpawnerInstanceCameraRefresh(EditorTestHelper):
    def __init__(self):
        EditorTestHelper.__init__(self, log_prefix="LayerSpawner_InstanceCameraRefresh", args=["level"])

    def run_test(self):
        """
        Summary:
        Test that the Dynamic Vegetation System is using the current Editor viewport camera as the center
        of the spawn area for vegetation.  To verify this, we create two separate Editor viewports pointed
        at two different vegetation areas, and verify that as we switch between active viewports, only the
        area directly underneath that viewport's camera has vegetation.
        """
        # Create an empty level
        self.test_success = self.create_level(
            self.args["level"],
            heightmap_resolution=128, 
            heightmap_meters_per_pixel=1, 
            terrain_texture_resolution=4096,
            use_terrain=False,
        )
        # Set up a test environment to validate that switching viewports correctly changes which camera
        # the vegetation system uses.
        # The test environment consists of the following:
        # - two 32 x 32 x 1 box shapes located far apart that emit a surface with no tags
        # - two 32 x 32 x 32 vegetation areas that place vegetation on the boxes

        # Initialize some constants for our test.
        # The boxes are intentionally shifted by 0.5 meters to ensure that we get a predictable number
        # of vegetation points.  By default, vegetation plants on grid corners, so if our boxes are aligned
        # with grid corner points, the right/bottom edges will include more points than we might intuitively expect.
        # By shifting by 0.5 meters, the vegetation grid points don't fall on the box edges, making the total count
        # more predictable.
        first_entity_center_point = math.Vector3(0.5, 0.5, 100.0)
        # The second box needs to be far enough away from the first that the vegetation system will never spawn instances
        # in both at the same time.
        second_entity_center_point = math.Vector3(1024.5, 1024.5, 100.0)
        box_size = 32.0
        surface_height = 1.0
        # By default, vegetation spawns 20 instances per 16 meters, so for our box of 32 meters, we should have
        # ((20 instances / 16 m) * 32 m) ^ 2 instances.
        filled_vegetation_area_instance_count = (20 * 2) * (20 * 2)

        # Change the Editor view to contain two viewports
        general.set_view_pane_layout(1)
        get_view_pane_layout_success = self.wait_for_condition(lambda: (general.get_view_pane_layout() == 1), 2)
        get_viewport_count_success = self.wait_for_condition(lambda: (general.get_viewport_count() == 2), 2)
        self.test_success = get_view_pane_layout_success and self.test_success
        self.test_success = get_viewport_count_success and self.test_success

        # Set the view in the first viewport to point down at the first box
        general.set_active_viewport(0)
        self.wait_for_condition(lambda: general.get_active_viewport() == 0, 2)
        general.set_current_view_position(first_entity_center_point.x, first_entity_center_point.y,
                                          first_entity_center_point.z + 30.0)
        general.set_current_view_rotation(-85.0, 0.0, 0.0)

        # Set the view in the second viewport to point down at the second box
        general.set_active_viewport(1)
        self.wait_for_condition(lambda: general.get_active_viewport() == 1, 2)
        general.set_current_view_position(second_entity_center_point.x, second_entity_center_point.y,
                                          second_entity_center_point.z + 30.0)
        general.set_current_view_rotation(-85.0, 0.0, 0.0)

        # Create the "flat surface" entities to use as our vegetation surfaces
        first_surface_entity = dynveg.create_surface_entity("Surface 1", first_entity_center_point, box_size, box_size,
                                                            surface_height)
        second_surface_entity = dynveg.create_surface_entity("Surface 2", second_entity_center_point, box_size, box_size,
                                                             surface_height)

        # Create the two vegetation areas
        test_slice_asset_path = os.path.join("Slices", "PurpleFlower.dynamicslice")
        first_veg_entity = dynveg.create_vegetation_area("Veg Area 1", first_entity_center_point, box_size, box_size,
                                                         box_size, test_slice_asset_path)
        second_veg_entity = dynveg.create_vegetation_area("Veg Area 2", second_entity_center_point, box_size, box_size,
                                                          box_size, test_slice_asset_path)

        # When the first viewport is active, the first area should be full of instances, and the second should be empty
        general.set_active_viewport(0)
        viewport_0_success = self.wait_for_condition(lambda: dynveg.validate_instance_count(first_entity_center_point,
                                                                                            box_size / 2.0,
                                                                                            filled_vegetation_area_instance_count), 5)
        self.test_success = viewport_0_success and self.test_success
        viewport_1_success = self.wait_for_condition(lambda: dynveg.validate_instance_count(second_entity_center_point,
                                                                                            box_size / 2.0, 0), 5)
        self.test_success = viewport_1_success and self.test_success

        # When the second viewport is active, the second area should be full of instances, and the first should be empty
        general.set_active_viewport(1)
        viewport_0_success = self.wait_for_condition(lambda: dynveg.validate_instance_count(first_entity_center_point,
                                                                                            box_size / 2.0, 0), 5)
        self.test_success = viewport_0_success and self.test_success
        viewport_1_success = self.wait_for_condition(lambda: dynveg.validate_instance_count(second_entity_center_point,
                                                                                            box_size / 2.0,
                                                                                            filled_vegetation_area_instance_count), 5)
        self.test_success = viewport_1_success and self.test_success


test = TestLayerSpawnerInstanceCameraRefresh()
test.run()
