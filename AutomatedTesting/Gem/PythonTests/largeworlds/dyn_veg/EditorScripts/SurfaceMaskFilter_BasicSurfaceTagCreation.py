"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import os
import sys

import azlmbr.surface_data as surface_data

sys.path.append(os.path.join(azlmbr.paths.devroot, 'AutomatedTesting', 'Gem', 'PythonTests'))
from editor_python_test_tools.editor_test_helper import EditorTestHelper


class TestSurfaceMaskFilter_BasicSurfaceTagCreation(EditorTestHelper):
    def __init__(self):
        EditorTestHelper.__init__(self, log_prefix="TestSurfaceMaskFilter_BasicSurfaceTagCreation", args=["level"])
        
    def run_test(self):
        """
        Summary:
        Verifies basic surface tag value equality

        Expected Behavior:
        Surface tags of the same name are equal, and different names aren't.

        Test Steps:
         1) Open level
         2) Create 2 new surface tags of identical names and verify they resolve as equal.
         3) Create another new tag of a different name and verify they resolve as different.

        Note:
        - This test file must be called from the Open 3D Engine Editor command terminal
        - Any passed and failed tests are written to the Editor.log file.
                Parsing the file or running a log_monitor are required to observe the test results.

        :return: None
        """
        self.log("SurfaceTag test started")
        
        # Create a level
        self.test_success = self.create_level(
            self.args["level"],
            heightmap_resolution=1024,
            heightmap_meters_per_pixel=1,
            terrain_texture_resolution=4096,
            use_terrain=False,
        )
        
        tag1 = surface_data.SurfaceTag()
        tag2 = surface_data.SurfaceTag()
        
        # Test 1:  Verify that two tags with the same value are equal
        tag1.SetTag('equal_test')
        tag2.SetTag('equal_test')
        self.log("SurfaceTag equal tag comparison is {} expected True".format(tag1.Equal(tag2)))
        self.test_success = self.test_success and tag1.Equal(tag2)
        
        # Test 2:  Verify that two tags with different values are not equal
        tag2.SetTag('not_equal_test')
        self.log("SurfaceTag not equal tag comparison is {} expected False".format(tag1.Equal(tag2)))
        self.test_success = self.test_success and not tag1.Equal(tag2)
        
        self.log("SurfaceTag test finished")


test = TestSurfaceMaskFilter_BasicSurfaceTagCreation()
test.run()
