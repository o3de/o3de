"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


class Tests:
    initial_instance_count = (
        "Initial instance count is as expected",
        "Unexpected number of initial instances found"
    )
    blocked_instance_count = (
        "Expected number of instances found after configuring Blocker",
        "Unexpected number of instances found after configuring Blocker"
    )



def LayerBlocker_InstancesBlockedInConfiguredArea():
    """
    Summary:
    An empty level is created. A Vegetation Layer Spawner area is configured. A Vegetation Layer Blocker area is
    configured to block instances in the spawner area.

    Expected Behavior:
    Vegetation is blocked by the configured Blocker area.

    Test Steps:
    1. A simple level is opened
    2. Vegetation Layer Spawner area is created
    3. Planting surface is created
    4. Vegetation System Settings level component is added, and Snap Mode set to center to ensure expected instance
    counts are accurate in the configured vegetation area
    5. Initial instance counts pre-blocker are validated
    6. A Vegetation Layer Blocker area is created, overlapping the spawner area
    7. Post-blocker instance counts are validated

    Note:
    - This test file must be called from the Open 3D Engine Editor command terminal
    - Any passed and failed tests are written to the Editor.log file.
            Parsing the file or running a log_monitor are required to observe the test results.

    :return: None
    """

    import os

    import azlmbr.bus as bus
    import azlmbr.editor as editor
    import azlmbr.math as math
    import azlmbr.legacy.general as general

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

    # 2) Create a new instance spawner entity
    spawner_center_point = math.Vector3(512.0, 512.0, 32.0)
    asset_path = os.path.join("Slices", "PinkFlower.dynamicslice")
    spawner_entity = dynveg.create_vegetation_area("Instance Spawner", spawner_center_point, 16.0, 16.0, 16.0,
                                                   asset_path)

    # 3) Create surface for planting on
    dynveg.create_surface_entity("Surface Entity", spawner_center_point, 32.0, 32.0, 1.0)

    # 4) Add a Vegetation System Settings Level component and set Sector Point Snap Mode to Center
    veg_system_settings_component = hydra.add_level_component("Vegetation System Settings")
    editor.EditorComponentAPIBus(bus.Broadcast, "SetComponentProperty", veg_system_settings_component,
                                 'Configuration|Area System Settings|Sector Point Snap Mode', 1)

    # 5) Verify initial instance counts
    num_expected = 20 * 20
    success = helper.wait_for_condition(lambda: dynveg.validate_instance_count_in_entity_shape(spawner_entity.id,
                                                                                             num_expected), 5.0)
    Report.result(Tests.initial_instance_count, success)

    # 6) Create a new Vegetation Layer Blocker area overlapping the spawner area
    blocker_entity = hydra.Entity("Blocker Area")
    blocker_entity.create_entity(
        spawner_center_point,
        ["Vegetation Layer Blocker", "Box Shape"]
    )
    if blocker_entity.id.IsValid():
        print(f"'{blocker_entity.name}' created")
    blocker_entity.get_set_test(1, "Box Shape|Box Configuration|Dimensions",
                                math.Vector3(3.0, 3.0, 3.0))

    # 7) Validate instance counts post-blocker. 16 instances should now be blocked in the center of the spawner area
    num_expected = (20 * 20) - 16
    success = helper.wait_for_condition(lambda: dynveg.validate_instance_count_in_entity_shape(spawner_entity.id,
                                                                                             num_expected), 5.0)
    Report.result(Tests.blocked_instance_count, success)


if __name__ == "__main__":

    from editor_python_test_tools.utils import Report
    Report.start_test(LayerBlocker_InstancesBlockedInConfiguredArea)
