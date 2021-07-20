"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import os
import sys

import azlmbr.bus as bus
import azlmbr.editor as editor
import azlmbr.entity as entity
import azlmbr.paths

sys.path.append(os.path.join(azlmbr.paths.devroot, 'AutomatedTesting', 'Gem', 'PythonTests'))
import editor_python_test_tools.hydra_editor_utils as hydra
from editor_python_test_tools.editor_test_helper import EditorTestHelper


class Scoped:
    def __init__(self, constructor, destructor, *args):
        self.data = constructor(*args)
        self.destructor = destructor

    def __del__(self):
        self.destructor(self.data)


class TestParams:
    def __init__(self, required_components, accessed_component):
        self.required_components = required_components
        self.accessed_component = accessed_component


class TestGradientPreviewSettings(EditorTestHelper):
    def __init__(self):
        EditorTestHelper.__init__(self, log_prefix="GradientPreviewSettings_DefaultPinnedEntity", args=["level"])

    def run_test(self):
        """
        Summary:
        This test verifies default values for the pinned entity for Gradient Preview settings.

        Expected Behavior:
        Pinned entity is self for all gradient generator/modifiers.

        Test Steps:
         1) Create a new level
         2) Create a new entity in the level
         3) Add each Gradient Generator component to an entity, and verify the Pin Preview to Shape property is set to
         self

        Note:
        - This test file must be called from the Open 3D Engine Editor command terminal
        - Any passed and failed tests are written to the Editor.log file.
                Parsing the file or running a log_monitor are required to observe the test results.

        :return: None
        """

        def execute_test(test_id, function, *args):
            if function(*args):
                self.log(test_id + ' has Preview pinned to own Entity result: SUCCESS')

        def create_entity():
            return editor.ToolsApplicationRequestBus(bus.Broadcast, 'CreateNewEntity', entity.EntityId())

        def delete_entity(entity_id):
            editor.ToolsApplicationRequestBus(bus.Broadcast, 'DeleteEntityAndAllDescendants', entity_id)

        def attach_components(component_list, entity_id):
            components = []
            for i in component_list:
                components.append(hydra.add_component(i, entity_id))
            return components

        def validate_id_is_current(param):
            entity_ptr = Scoped(create_entity, delete_entity)
            added_components = attach_components(param.required_components, entity_ptr.data)
            value = hydra.get_component_property_value(added_components[param.accessed_component],
                'Preview Settings|Pin Preview to Shape')
            self.test_success = self.test_success and entity_ptr.data.Equal(value)
            return entity_ptr.data.Equal(value)

        param_list = [
            TestParams(['Gradient Transform Modifier', 'Box Shape', 'Perlin Noise Gradient'], 2),
            TestParams(['Random Noise Gradient', 'Gradient Transform Modifier', 'Box Shape'], 0),
            TestParams(['FastNoise Gradient', 'Gradient Transform Modifier', 'Box Shape'], 0),
            TestParams(['Dither Gradient Modifier'], 0),
            TestParams(['Invert Gradient Modifier'], 0),
            TestParams(['Levels Gradient Modifier'], 0),
            TestParams(['Posterize Gradient Modifier'], 0),
            TestParams(['Smooth-Step Gradient Modifier'], 0),
            TestParams(['Threshold Gradient Modifier'], 0)
        ]

        # Create a new empty level
        self.test_success = self.create_level(
            self.args["level"],
            heightmap_resolution=1024,
            heightmap_meters_per_pixel=1,
            terrain_texture_resolution=4096,
            use_terrain=False,
        )

        for param in param_list:
            execute_test(param.required_components[param.accessed_component],
                         validate_id_is_current, param)


test = TestGradientPreviewSettings()
test.run()
