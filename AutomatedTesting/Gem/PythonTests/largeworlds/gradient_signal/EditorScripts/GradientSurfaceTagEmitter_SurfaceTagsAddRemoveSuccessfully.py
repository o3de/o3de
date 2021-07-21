"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import os
import sys

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import azlmbr.math as math
import azlmbr.paths
import azlmbr.surface_data as surface_data

sys.path.append(os.path.join(azlmbr.paths.devroot, 'AutomatedTesting', 'Gem', 'PythonTests'))
import editor_python_test_tools.hydra_editor_utils as hydra
from editor_python_test_tools.editor_test_helper import EditorTestHelper


class TestGradientSurfaceTagEmitter(EditorTestHelper):
    def __init__(self):
        EditorTestHelper.__init__(self, log_prefix="GradientSurfaceTagEmitter_SurfaceTagsAddRemoveSucessfully",
                                  args=["level"])

    def run_test(self):
        """
        Summary:
        Entity with Gradient Surface Tag Emitter and Reference Gradient components is created. 
        And new surface tag has been added and removed.
        
        Expected Behavior:
        A new Surface Tag can be added and removed from the component

        Test Steps:
        1) Open level
        2) Create an entity with Gradient Surface Tag Emitter and Reference Gradient components.
        3) Add/ remove Surface Tags

        Note:
        - This test file must be called from the Open 3D Engine Editor command terminal
        - Any passed and failed tests are written to the Editor.log file.
                Parsing the file or running a log_monitor are required to observe the test results.

        :return: None
        """

        # 1) Open level
        self.test_success = self.create_level(
            self.args["level"],
            heightmap_resolution=1024,
            heightmap_meters_per_pixel=1,
            terrain_texture_resolution=4096,
            use_terrain=False,
        )

        # 2) Create an entity with Gradient Surface Tag Emitter and Reference Gradient components.
        entity_position = math.Vector3(125.0, 136.0, 32.0)
        components_to_add = ["Gradient Surface Tag Emitter", "Reference Gradient"]
        entity = hydra.Entity("entity")
        entity.create_entity(entity_position, components_to_add)

        # 3) Add/ remove Surface Tags
        tag = surface_data.SurfaceTag()
        tag.SetTag("water")
        pte = hydra.get_property_tree(entity.components[0])
        path = "Configuration|Extended Tags"
        pte.add_container_item(path, 0, tag)
        success = self.wait_for_condition(lambda: pte.get_container_count(path).GetValue() == 1)
        self.test_success = self.test_success and success
        print(f"Added SurfaceTag: container count is {pte.get_container_count(path).GetValue()}")
        pte.remove_container_item(path, 0)
        success = self.wait_for_condition(lambda: pte.get_container_count(path).GetValue() == 0)
        self.test_success = self.test_success and success
        print(f"Removed SurfaceTag: container count is {pte.get_container_count(path).GetValue()}")


test = TestGradientSurfaceTagEmitter()
test.run()
