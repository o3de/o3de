"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import os
import sys
import azlmbr.math as math
import azlmbr.paths
import azlmbr.editor as editor
import azlmbr.bus as bus
import azlmbr.legacy.general as general

sys.path.append(os.path.join(azlmbr.paths.devroot, "AutomatedTesting", "Gem", "PythonTests"))
import editor_python_test_tools.hydra_editor_utils as hydra
from editor_python_test_tools.editor_test_helper import EditorTestHelper
from largeworlds.large_worlds_utils import editor_dynveg_test_helper as dynveg


class TestSystemSettingsSectorSize(EditorTestHelper):
    def __init__(self):
        EditorTestHelper.__init__(self, log_prefix="SystemSettings_SectorSize", args=["level"])

    def run_test(self):
        """
        Summary:
        Sector Size In Meters increases/reduces the size of a sector

        Expected Result:
        The number of spawned vegetation meshes inside the vegetation area is identical after updating the Sector Size

        :return: None
        """

        VEGETATION_INSTANCE_COUNT = 400

        # Create empty level
        self.test_success = self.create_level(
            self.args["level"],
            heightmap_resolution=1024,
            heightmap_meters_per_pixel=1,
            terrain_texture_resolution=4096,
            use_terrain=False,
        )

        general.set_current_view_position(512.0, 480.0, 38.0)

        # Create basic vegetation entity
        position = math.Vector3(512.0, 512.0, 32.0)
        asset_path = os.path.join("Slices", "PinkFlower.dynamicslice")
        vegetation = dynveg.create_vegetation_area("vegetation", position, 16.0, 16.0, 1.0, asset_path)
        dynveg.create_surface_entity("Surface_Entity", position, 16.0, 16.0, 1.0)

        # Add the Vegetation Debugger component to the Level Inspector
        veg_system_settings_component = hydra.add_level_component("Vegetation System Settings")

        # Count the number of vegetation meshes along one side of the new vegetation area.
        result = self.wait_for_condition(
            lambda: dynveg.validate_instance_count(position, 8.0, VEGETATION_INSTANCE_COUNT), 2.0
        )
        self.log(f"Vegetation instances count equal to expected value before changing sector size: {result}")

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

        # Count the number of vegetation meshes along one side of the new vegetation area.
        result = self.wait_for_condition(
            lambda: dynveg.validate_instance_count(position, 5.0, VEGETATION_INSTANCE_COUNT), 2.0
        )
        self.log(f"Vegetation instances count equal to expected value after changing sector size: {result}")


test = TestSystemSettingsSectorSize()
test.run()
