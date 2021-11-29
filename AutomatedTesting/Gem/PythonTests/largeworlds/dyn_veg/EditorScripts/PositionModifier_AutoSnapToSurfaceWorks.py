"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


class Tests:
    initial_instance_count = (
        "Initial instance count is as expected",
        "Found an unexpected number of initial instances"
    )
    autosnap_enabled_instance_count = (
        "Found the expected number of instances with Auto Snap to Surface enabled",
        "Found an unexpected number of instances with Auto Snap to Surface enabled"
    )
    autosnap_disabled_instance_count = (
        "Found the expected number of instances with Auto Snap to Surface disabled",
        "Found an unexpected number of instances with Auto Snap to Surface disabled"
    )


def PositionModifier_AutoSnapToSurfaceWorks():
    """
    Summary:
    Instance spawner is setup to plant on a spherical mesh. Offsets are set on the x-axis, and checks are performed
    to ensure instances plant where expected depending on the toggle setting.

    Expected Behavior:
    Offset instances snap to the expected surface when Auto Snap to Surface is enabled, and offset away from surface
    when it is disabled.

    Test Steps:
     1) Open a simple level
     2) Create a new entity with required vegetation area components and a Position Modifier
     3) Create a spherical planting surface
     4) Verify initial instance counts pre-filter
     5) Create a child entity of the spawner entity with a Constant Gradient component and pin to spawner
     6) Set the Position Modifier offset to 5 on the x-axis
     7) Validate instance counts on top of and inside the sphere mesh with Auto Snap to Surface enabled
     8) Validate instance counts on top of and inside the sphere mesh with Auto Snap to Surface disabled

    Note:
    - This test file must be called from the Open 3D Engine Editor command terminal
    - Any passed and failed tests are written to the Editor.log file.
            Parsing the file or running a log_monitor are required to observe the test results.

    :return: None
    """

    import os

    import azlmbr.legacy.general as general
    import azlmbr.math as math

    import editor_python_test_tools.hydra_editor_utils as hydra
    from largeworlds.large_worlds_utils import editor_dynveg_test_helper as dynveg
    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper as helper

    position_modifier_paths = ['Configuration|Position X|Range Min', 'Configuration|Position X|Range Max',
                               'Configuration|Position Y|Range Min', 'Configuration|Position Y|Range Max',
                               'Configuration|Position Z|Range Min', 'Configuration|Position Z|Range Max']

    # 1) Open an existing simple level
    helper.init_idle()
    helper.open_level("Physics", "Base")

    # Set view of planting area for visual debugging
    general.set_current_view_position(512.0, 500.0, 38.0)
    general.set_current_view_rotation(-20.0, 0.0, 0.0)

    # 2) Create a new entity with required vegetation area components and a Position Modifier
    spawner_center_point = math.Vector3(512.0, 512.0, 32.0)
    asset_path = os.path.join("Slices", "PinkFlower.dynamicslice")
    spawner_entity = dynveg.create_vegetation_area("Instance Spawner", spawner_center_point, 16.0, 16.0, 16.0,
                                                   asset_path)

    # Add a Vegetation Position Modifier and set offset values to 0
    spawner_entity.add_component("Vegetation Position Modifier")
    for path in position_modifier_paths:
        spawner_entity.get_set_test(3, path, 0)

    # 3) Create a spherical planting surface
    hill_entity = dynveg.create_mesh_surface_entity_with_slopes("Planting Surface", spawner_center_point, 5.0)

    # 4) Verify initial instance counts pre-filter
    num_expected = 29
    spawner_success = helper.wait_for_condition(
        lambda: dynveg.validate_instance_count_in_entity_shape(spawner_entity.id, num_expected), 5.0)
    Report.result(Tests.initial_instance_count, spawner_success)

    # 5) Create a child entity of the spawner entity with a Constant Gradient component and pin to spawner
    components_to_add = ["Constant Gradient"]
    gradient_entity = hydra.Entity("Gradient Entity")
    gradient_entity.create_entity(spawner_center_point, components_to_add, parent_id=spawner_entity.id)

    # Pin the Constant Gradient to the X axis of the spawner's Position Modifier component
    spawner_entity.get_set_test(3, 'Configuration|Position X|Gradient|Gradient Entity Id', gradient_entity.id)

    # 6) Set the Position Modifier offset to 2.5 on the x-axis
    spawner_entity.get_set_test(3, position_modifier_paths[0], 2.5)
    spawner_entity.get_set_test(3, position_modifier_paths[1], 2.5)

    # 7) Validate instance count at the top of the sphere mesh and inside the sphere mesh while Auto Snap to Surface
    # is enabled
    top_point = math.Vector3(512.0, 512.0, 37.0)
    inside_point = math.Vector3(512.0, 512.0, 35.0)
    radius = 0.5
    num_expected = 1
    Report.info(f"Checking for instances in a {radius * 2}m area at {top_point.ToString()}")
    top_success = helper.wait_for_condition(lambda: dynveg.validate_instance_count(top_point, radius, num_expected),
                                          5.0)
    num_expected = 0
    Report.info(f"Checking for instances in a {radius * 2}m area at {inside_point.ToString()}")
    inside_success = helper.wait_for_condition(lambda: dynveg.validate_instance_count(inside_point, radius,
                                                                                      num_expected), 5.0)
    Report.result(Tests.autosnap_enabled_instance_count, top_success and inside_success)

    # 8) Toggle off Auto Snap to Surface. Instances should now plant inside the sphere and no longer on top
    spawner_entity.get_set_test(3, "Configuration|Auto Snap To Surface", False)
    num_expected = 0
    Report.info(f"Checking for instances in a {radius * 2}m area at {top_point.ToString()}")
    top_success = helper.wait_for_condition(lambda: dynveg.validate_instance_count(top_point, radius, num_expected),
                                            5.0)
    num_expected = 1
    Report.info(f"Checking for instances in a {radius * 2}m area at {inside_point.ToString()}")
    inside_success = helper.wait_for_condition(lambda: dynveg.validate_instance_count(inside_point, radius,
                                                                                    num_expected), 5.0)
    Report.result(Tests.autosnap_disabled_instance_count, top_success and inside_success)


if __name__ == "__main__":

    from editor_python_test_tools.utils import Report
    Report.start_test(PositionModifier_AutoSnapToSurfaceWorks)
