"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


class Tests:
    viewport_config_updated = (
        "Viewport is now configured for test",
        "Failed to configure viewport for test"
    )
    first_viewport_active_instance_count = (
        "Expected number of instances found in left viewport",
        "Unexpected number of instances found in left viewport"
    )
    second_viewport_inactive_instance_count = (
        "No instances found in right viewport",
        "Unexpectedly found instances in right viewport while not active"
    )
    first_viewport_inactive_instance_count = (
        "No instances found in left viewport",
        "Unexpectedly found instances in left viewport while not active"
    )
    second_viewport_active_instance_count = (
        "Expected number of instances found in right viewport",
        "Unexpected number of instances found in right viewport"
    )


def LayerSpawner_InstancesRefreshUsingCorrectViewportCamera():
    """
    Summary:
    Test that the Dynamic Vegetation System is using the current Editor viewport camera as the center
    of the spawn area for vegetation.  To verify this, we create two separate Editor viewports pointed
    at two different vegetation areas, and verify that as we switch between active viewports, only the
    area directly underneath that viewport's camera has vegetation.
    """

    import os

    import azlmbr.legacy.general as general
    import azlmbr.math as math

    from largeworlds.large_worlds_utils import editor_dynveg_test_helper as dynveg
    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper as helper

    # Open an existing simple level
    helper.init_idle()
    helper.open_level("Physics", "Base")

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
    get_view_pane_layout_success = helper.wait_for_condition(lambda: (general.get_view_pane_layout() == 1), 2)
    get_viewport_count_success = helper.wait_for_condition(lambda: (general.get_viewport_count() == 2), 2)
    Report.critical_result(Tests.viewport_config_updated, get_view_pane_layout_success and get_viewport_count_success)

    # Set the view in the first viewport to point down at the first box
    general.set_active_viewport(0)
    helper.wait_for_condition(lambda: general.get_active_viewport() == 0, 2)
    general.set_current_view_position(first_entity_center_point.x, first_entity_center_point.y,
                                      first_entity_center_point.z + 30.0)
    general.set_current_view_rotation(-85.0, 0.0, 0.0)

    # Set the view in the second viewport to point down at the second box
    general.set_active_viewport(1)
    helper.wait_for_condition(lambda: general.get_active_viewport() == 1, 2)
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
    helper.wait_for_condition(lambda: general.get_active_viewport() == 0, 2)
    viewport_0_success = helper.wait_for_condition(lambda: dynveg.validate_instance_count(first_entity_center_point,
                                                                                        box_size / 2.0,
                                                                                        filled_vegetation_area_instance_count), 5)
    viewport_1_success = helper.wait_for_condition(lambda: dynveg.validate_instance_count(second_entity_center_point,
                                                                                        box_size / 2.0, 0), 5)
    Report.result(Tests.first_viewport_active_instance_count, viewport_0_success)
    Report.result(Tests.second_viewport_inactive_instance_count, viewport_1_success)

    # When the second viewport is active, the second area should be full of instances, and the first should be empty
    general.set_active_viewport(1)
    helper.wait_for_condition(lambda: general.get_active_viewport() == 1, 2)
    viewport_0_success = helper.wait_for_condition(lambda: dynveg.validate_instance_count(first_entity_center_point,
                                                                                        box_size / 2.0, 0), 5)
    Report.result(Tests.first_viewport_inactive_instance_count, viewport_0_success)
    viewport_1_success = helper.wait_for_condition(lambda: dynveg.validate_instance_count(second_entity_center_point,
                                                                                        box_size / 2.0,
                                                                                        filled_vegetation_area_instance_count), 5)
    Report.result(Tests.second_viewport_active_instance_count, viewport_1_success)


if __name__ == "__main__":

    from editor_python_test_tools.utils import Report
    Report.start_test(LayerSpawner_InstancesRefreshUsingCorrectViewportCamera)
