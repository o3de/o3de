"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


class Tests:
    instance_validation_close = (
        "Instance count is as expected when within range of Spawner",
        "Instance count was unexpected when within range of Spawner"
    )
    instance_validation_far = (
        "No instances found when out of range of Spawner",
        "Instances still found when out of range of Spawner"
    )


def VegetationInstances_DespawnWhenOutOfRange():
    """
    Summary:
    Verifies that vegetation instances properly spawn/despawn based on camera range.

    Expected Behavior:
    Vegetation instances despawn when out of camera range.

    Test Steps:
     1) Open a simple level
     2) Create a simple vegetation area, and set the view position near the spawner. Verify instances plant.
     3) Move the view position away from the spawner. Verify instances despawn.

    Note:
    - This test file must be called from the Open 3D Engine Editor command terminal
    - Any passed and failed tests are written to the Editor.log file.
            Parsing the file or running a log_monitor are required to observe the test results.

    :return: None
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
    result = helper.wait_for_condition(lambda: dynveg.validate_instance_count_in_entity_shape(spawner_entity.id,
                                                                                            num_expected), 2.0)
    Report.result(Tests.instance_validation_close, result)

    # Move sufficiently far away from the veg area that it should all despawn.
    general.set_current_view_position(position.x - 1000.0, position.y - 1000.0, position.z + 30.0)

    # We now expect to find 0 instances.  If the bug exists, we will find 400 still.
    num_expected = 0
    result = helper.wait_for_condition(lambda: dynveg.validate_instance_count_in_entity_shape(spawner_entity.id,
                                                                                            num_expected), 2.0)
    Report.result(Tests.instance_validation_far, result)


if __name__ == "__main__":

    from editor_python_test_tools.utils import Report
    Report.start_test(VegetationInstances_DespawnWhenOutOfRange)
