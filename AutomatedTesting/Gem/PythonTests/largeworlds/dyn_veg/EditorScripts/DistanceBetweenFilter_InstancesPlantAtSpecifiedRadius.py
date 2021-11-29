"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


class Tests:
    initial_instance_counts = (
        "Initial instance counts are as expected",
        "Unexpected number of initial instances found"
    )
    instance_counts_1m = (
        "Instance counts with 1 meters between instances are as expected",
        "Unexpected number of instances found with 1 meters between instances"
    )
    instance_counts_2m = (
        "Instance counts with 2 meters between instances are as expected",
        "Unexpected number of instances found with 2 meters between instances"
    )
    instance_counts_16m = (
        "Instance counts with 16 meters between instances are as expected",
        "Unexpected number of instances found with 16 meters between instances"
    )


def DistanceBetweenFilter_InstancesPlantAtSpecifiedRadius():
    """
    Summary: Creates a level with a simple vegetation area. A Vegetation Distance Between Filter is
        added and the min radius is changed. Instance counts at specific points are validated.

    Test Steps:
    1) Open a simple level
    2) Create a vegetation area
    3) Create a surface for planting
    4) Add the Vegetation System Settings component and setup for the test
    5-8) Add the Distance Between Filter, and validate expected instance counts with a few different Radius values

    Note:
    - This test file must be called from the Open 3D Engine Editor command terminal
    - Any passed and failed tests are written to the Editor.log file.
            Parsing the file or running a log_monitor are required to observe the test results.

    :return: None
    """

    import os

    import azlmbr.editor as editor
    import azlmbr.legacy.general as general
    import azlmbr.bus as bus
    import azlmbr.math as math

    import editor_python_test_tools.hydra_editor_utils as hydra
    from largeworlds.large_worlds_utils import editor_dynveg_test_helper as dynveg
    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper as helper

    instance_query_point_a = math.Vector3(512.5, 512.5, 32.0)
    instance_query_point_b = math.Vector3(514.0, 512.5, 32.0)
    instance_query_point_c = math.Vector3(515.0, 512.5, 32.0)

    # 1) Open an existing simple level
    helper.init_idle()
    helper.open_level("Physics", "Base")

    general.set_current_view_position(512.0, 480.0, 38.0)

    # 2) Create a new entity with required vegetation area components
    spawner_center_point = math.Vector3(520.0, 520.0, 32.0)
    asset_path = os.path.join("Slices", "1m_cube.dynamicslice")
    spawner_entity = dynveg.create_vegetation_area("Instance Spawner", spawner_center_point, 16.0, 16.0, 16.0,
                                                   asset_path)

    # 3) Create a surface to plant on
    surface_center_point = math.Vector3(512.0, 512.0, 32.0)
    dynveg.create_surface_entity("Planting Surface", surface_center_point, 128.0, 128.0, 1.0)

    # 4) Add a Vegetation System Settings Level component and set Sector Point Snap Mode to Center
    veg_system_settings_component = hydra.add_level_component("Vegetation System Settings")
    editor.EditorComponentAPIBus(bus.Broadcast, "SetComponentProperty", veg_system_settings_component,
                                 'Configuration|Area System Settings|Sector Point Snap Mode', 1)
    editor.EditorComponentAPIBus(bus.Broadcast, "SetComponentProperty", veg_system_settings_component,
                                 'Configuration|Area System Settings|Sector Point Density', 16)

    # 5) Add a Vegetation Distance Between Filter and verify initial instance counts are accurate
    spawner_entity.add_component("Vegetation Distance Between Filter")
    num_expected = 16 * 16
    initial_success = helper.wait_for_condition(lambda: dynveg.validate_instance_count_in_entity_shape(spawner_entity.id, num_expected), 5.0)
    Report.result(Tests.initial_instance_counts, initial_success)

    # 6) Change Radius Min to 1.0, refresh, and verify instance counts are accurate
    spawner_entity.get_set_test(3, "Configuration|Radius Min", 1.0)
    point_a_success = helper.wait_for_condition(lambda: dynveg.validate_instance_count(instance_query_point_a, 0.5, 1), 5.0)
    point_b_success = helper.wait_for_condition(lambda: dynveg.validate_instance_count(instance_query_point_b, 0.5, 0), 5.0)
    point_c_success = helper.wait_for_condition(lambda: dynveg.validate_instance_count(instance_query_point_c, 0.5, 1), 5.0)
    Report.result(Tests.instance_counts_1m, point_a_success and point_b_success and point_c_success)

    # 7) Change Radius Min to 2.0, refresh, and verify instance counts are accurate
    spawner_entity.get_set_test(3, "Configuration|Radius Min", 2.0)
    point_a_success = helper.wait_for_condition(lambda: dynveg.validate_instance_count(instance_query_point_a, 0.5, 1), 5.0)
    point_b_success = helper.wait_for_condition(lambda: dynveg.validate_instance_count(instance_query_point_b, 0.5, 0), 5.0)
    point_c_success = helper.wait_for_condition(lambda: dynveg.validate_instance_count(instance_query_point_c, 0.5, 0), 5.0)
    Report.result(Tests.instance_counts_2m, point_a_success and point_b_success and point_c_success)

    # 8) Change Radius Min to 16.0, refresh, and verify instance counts are accurate
    spawner_entity.get_set_test(3, "Configuration|Radius Min", 16.0)
    num_expected_instances = 1
    final_check_success = helper.wait_for_condition(lambda: dynveg.validate_instance_count_in_entity_shape(spawner_entity.id, num_expected_instances), 5.0)
    Report.result(Tests.instance_counts_16m, final_check_success)


if __name__ == "__main__":

    from editor_python_test_tools.utils import Report
    Report.start_test(DistanceBetweenFilter_InstancesPlantAtSpecifiedRadius)
