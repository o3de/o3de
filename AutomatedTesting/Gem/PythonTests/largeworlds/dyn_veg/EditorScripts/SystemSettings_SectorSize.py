"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


class Tests:
    initial_sector_size_instance_count = (
        "Found the expected number of instances with default sector size",
        "Found an unexpected number of instances with default sector size"
    )
    configured_sector_size_instance_count = (
        "Found the expected number of instances with a sector size of 10",
        "Found an unexpected number of instances with a sector size of 10"
    )


def SystemSettings_SectorSize():
    """
    Summary:
    Sector Size In Meters increases/reduces the size of a sector

    Expected Result:
    The number of spawned vegetation meshes inside the vegetation area is identical after updating the Sector Size

    :return: None
    """

    import os

    import azlmbr.math as math
    import azlmbr.editor as editor
    import azlmbr.bus as bus
    import azlmbr.legacy.general as general

    import editor_python_test_tools.hydra_editor_utils as hydra
    from largeworlds.large_worlds_utils import editor_dynveg_test_helper as dynveg
    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper as helper

    VEGETATION_INSTANCE_COUNT = 400

    # Open an existing simple level
    hydra.open_base_level()

    general.set_current_view_position(512.0, 480.0, 38.0)

    # Create basic vegetation entity
    position = math.Vector3(512.0, 512.0, 32.0)
    pink_flower_asset_path = os.path.join("assets", "objects", "foliage", "grass_flower_pink.fbx.azmodel")
    pink_flower_prefab = dynveg.create_temp_mesh_prefab(pink_flower_asset_path, "SectorSize_PinkFlower")[0]
    vegetation = dynveg.create_temp_prefab_vegetation_area("Instance Spawner", position, 16.0, 16.0, 1.0,
                                                               pink_flower_prefab)
    dynveg.create_surface_entity("Surface_Entity", position, 16.0, 16.0, 1.0)

    # Add the Vegetation Debugger component to the Level Inspector
    veg_system_settings_component = hydra.add_level_component("Vegetation System Settings")

    # Count the number of vegetation instances in the vegetation area
    result = helper.wait_for_condition(
        lambda: dynveg.validate_instance_count(position, 8.0, VEGETATION_INSTANCE_COUNT), 2.0
    )
    Report.result(Tests.initial_sector_size_instance_count, result)

    # Change Sector Size in Meters to 10.
    editor.EditorComponentAPIBus(
        bus.Broadcast,
        "SetComponentProperty",
        veg_system_settings_component,
        "Configuration|Area System Settings|Sector Point Snap Mode",
        1,
    )
    editor.EditorComponentAPIBus(
        bus.Broadcast,
        "SetComponentProperty",
        veg_system_settings_component,
        "Configuration|Area System Settings|Sector Size In Meters",
        10,
    )

    # Alter the Box Shape to be 10,10,1
    vegetation.get_set_test(1, "Box Shape|Box Configuration|Dimensions", math.Vector3(10.0, 10.0, 1.0))

    # Count the number of vegetation instances in the vegetation area
    result = helper.wait_for_condition(
        lambda: dynveg.validate_instance_count(position, 5.0, VEGETATION_INSTANCE_COUNT), 2.0
    )
    Report.result(Tests.configured_sector_size_instance_count, result)


if __name__ == "__main__":

    from editor_python_test_tools.utils import Report
    Report.start_test(SystemSettings_SectorSize)
