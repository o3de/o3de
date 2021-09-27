"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


class Tests:
    initial_instance_count = (
        "Initial instance count is as expected",
        "Initial instance count does not match expected results"
    )
    layer_priority_instance_count = (
        "Instance count is as expected after updating layer priorities",
        "Instance count does not match expected results after updating layer priorities"
    )
    sub_priority_instance_count = (
        "Instance count is as expected after updating sub priorities",
        "Instance count does not match expected results after updating sub priorities"
    )


def InstanceSpawnerPriority_LayerAndSubPriority():
    """
    Summary:
    A new level is created. An instance spawner area and blocker area are setup to overlap. Instance counts are
    verified with the initial setup. Layer priority on the blocker area is set to lower than the instance spawner
    area, and instance counts are re-verified.

    Expected Behavior:
    Vegetation areas with a higher Layer Priority plant over those with a lower Layer Priority

    Test Steps:
     1) Open a simple level
     2) Create overlapping instance spawner and blocker areas
     3) Create a surface to plant on
     4) Validate initial instance counts in the spawner area
     5) Reduce the Layer Priority of the blocker area
     6) Validate instance counts in the spawner area

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

    # 1) Open an existing simple level
    helper.init_idle()
    helper.open_level("Physics", "Base")

    # Set view of planting area for visual debugging
    general.set_current_view_position(512.0, 500.0, 38.0)
    general.set_current_view_rotation(-20.0, 0.0, 0.0)

    # 2) Create overlapping areas: 1 instance spawner area, and 1 blocker area
    spawner_center_point = math.Vector3(508.0, 508.0, 32.0)
    asset_path = os.path.join("Slices", "PinkFlower.dynamicslice")
    spawner_entity = dynveg.create_vegetation_area("Instance Spawner", spawner_center_point, 16.0, 16.0, 1.0,
                                                   asset_path)
    blocker_center_point = math.Vector3(516.0, 516.0, 32.0)
    blocker_entity = dynveg.create_blocker_area("Instance Blocker", blocker_center_point, 16.0, 16.0, 1.0)

    # 3) Create a surface for planting
    planting_surface_center_point = math.Vector3(512.0, 512.0, 32.0)
    dynveg.create_surface_entity("Planting Surface", planting_surface_center_point, 64.0, 64.0, 1.0)

    # Set instances to spawn on a center snap point to avoid unexpected instances around the edges of the box shape
    veg_system_settings_component = hydra.add_level_component("Vegetation System Settings")
    editor.EditorComponentAPIBus(bus.Broadcast, "SetComponentProperty", veg_system_settings_component,
                                 'Configuration|Area System Settings|Sector Point Snap Mode', 1)

    # 4) Validate the expected instance count with initial setup. GetAreaProductCount is used as
    # GetInstanceCountInAabb does not filter out blocked instances
    num_expected = (20 * 20) - (10 * 10)  # 20 instances per 16m per side minus 1 blocked quadrant
    result = helper.wait_for_condition(lambda: dynveg.validate_instance_count_in_entity_shape(spawner_entity.id,
                                                                                            num_expected), 5.0)
    Report.result(Tests.initial_instance_count, result)

    # 5) Change the Instance Spawner area to a higher layer priority than the Instance Blocker
    blocker_entity.get_set_test(0, 'Configuration|Layer Priority', 0)

    # 6) Validate the expected instance count with changed area priorities
    num_expected = 20 * 20  # 20 instances per 16m per side, no instances should be blocked at this point
    result = helper.wait_for_condition(lambda: dynveg.validate_instance_count_in_entity_shape(spawner_entity.id,
                                                                                            num_expected), 5.0)
    Report.result(Tests.layer_priority_instance_count, result)

    # 7) Revert Layer Priority changes so both areas are equal, and change Sub Priority to a higher value on the
    # Instance Spawner area
    blocker_entity.get_set_test(0, 'Configuration|Layer Priority', 1)
    spawner_entity.get_set_test(0, 'Configuration|Sub Priority', 100)
    blocker_entity.get_set_test(0, 'Configuration|Sub Priority', 1)

    # 8) Validate the expected instance count with changed area priorities
    num_expected = 20 * 20  # 20 instances per 16m per side, no instances should be blocked at this point
    result = helper.wait_for_condition(lambda: dynveg.validate_instance_count_in_entity_shape(spawner_entity.id,
                                                                                            num_expected), 5.0)
    Report.result(Tests.sub_priority_instance_count, result)


if __name__ == "__main__":

    from editor_python_test_tools.utils import Report
    Report.start_test(InstanceSpawnerPriority_LayerAndSubPriority)
