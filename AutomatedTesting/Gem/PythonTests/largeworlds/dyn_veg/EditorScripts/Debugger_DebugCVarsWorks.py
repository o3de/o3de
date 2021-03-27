"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
"""

import os
import sys

import azlmbr.legacy.general as general
import azlmbr.paths

sys.path.append(os.path.join(azlmbr.paths.devroot, "AutomatedTesting", "Gem", "PythonTests"))
import automatedtesting_shared.hydra_editor_utils as hydra
from automatedtesting_shared.editor_test_helper import EditorTestHelper


class TestDebuggerDebugCVarsWorks(EditorTestHelper):
    def __init__(self):
        EditorTestHelper.__init__(self, log_prefix="Debugger_DebugCVarsWorks", args=["level"])

    def run_test(self):
        """
        Summary:
        C2789148 Vegetation Debug CVars are enabled when the Debugger component is present

        Expected Result:
        The following commands are available in the Editor only when the Vegetation Debugger Level component is present:
        veg_debugDumpReport (Command)
        veg_debugRefreshAllAreas (Command)

        :return: None
        """

        # Create empty level
        self.test_success = self.create_level(
            self.args["level"],
            heightmap_resolution=1024,
            heightmap_meters_per_pixel=1,
            terrain_texture_resolution=4096,
            use_terrain=False,
        )

        # Initially run the command in console without Debugger component
        general.run_console("veg_debugDumpReport")

        # Add the Vegetation Debugger component to the Level Inspector
        hydra.add_level_component("Vegetation Debugger")

        # Run a command again after adding the Vegetation debugger
        general.run_console("veg_debugRefreshAllAreas")


test = TestDebuggerDebugCVarsWorks()
test.run()
