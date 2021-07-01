"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import os
import sys

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import azlmbr.bus as bus
import azlmbr.editor as editor
import azlmbr.entity as EntityId
import azlmbr.math as math
import azlmbr.paths

sys.path.append(os.path.join(azlmbr.paths.devroot, 'AutomatedTesting', 'Gem', 'PythonTests'))
import editor_python_test_tools.hydra_editor_utils as hydra
from editor_python_test_tools.editor_test_helper import EditorTestHelper


class TestMeshSurfaceTagEmitter(EditorTestHelper):
    def __init__(self):
        EditorTestHelper.__init__(self, log_prefix="MeshSurfaceTagEmitter_DependentOnMeshComponent", args=["level"])

    def run_test(self):
        """
        Summary:
        A New level is loaded. A New entity is created with component "Mesh Surface Tag Emitter". Adding a component
        "Mesh" to the same entity.

        Expected Behavior:
        Mesh Surface Tag Emitter is disabled until the required Mesh component is added to the entity.

        Test Steps:
         1) Open level
         2) Create a new entity with component "Mesh Surface Tag Emitter"
         3) Make sure Mesh Surface Tag Emitter is disabled
         4) Add Mesh to the same entity
         5) Make sure Mesh Surface Tag Emitter is enabled after adding Mesh

        Note:
        - This test file must be called from the Open 3D Engine Editor command terminal
        - Any passed and failed tests are written to the Editor.log file.
                Parsing the file or running a log_monitor are required to observe the test results.

        :return: None
        """

        def is_component_enabled(EntityComponentIdPair):
            return editor.EditorComponentAPIBus(bus.Broadcast, "IsComponentEnabled", EntityComponentIdPair)

        # 1) Open level
        self.test_success = self.create_level(
            self.args["level"],
            heightmap_resolution=1024,
            heightmap_meters_per_pixel=1,
            terrain_texture_resolution=4096,
            use_terrain=False,
        )

        # 2) Create a new entity with component "Mesh Surface Tag Emitter"
        entity_position = math.Vector3(125.0, 136.0, 32.0)
        component_to_add = "Mesh Surface Tag Emitter"
        entity_id = editor.ToolsApplicationRequestBus(
            bus.Broadcast, "CreateNewEntityAtPosition", entity_position, EntityId.EntityId()
        )
        meshentity = hydra.Entity("meshentity", entity_id)
        meshentity.components = []
        meshentity.components.append(hydra.add_component(component_to_add, entity_id))
        if entity_id.IsValid():
            print("New Entity Created")

        # 3) Make sure Mesh Surface Tag Emitter is disabled
        is_enabled = is_component_enabled(meshentity.components[0])
        self.test_success = self.test_success and not is_enabled
        if not is_enabled:
            print(f"{component_to_add} is Disabled")
        elif is_enabled:
            print(f"{component_to_add} is Enabled. But It should be disabled before adding Mesh")

        # 4) Add Mesh to the same entity
        component = "Mesh"
        meshentity.components.append(hydra.add_component(component, entity_id))

        # 5) Make sure Mesh Surface Tag Emitter is enabled after adding Mesh
        is_enabled = is_component_enabled(meshentity.components[0])
        self.test_success = self.test_success and is_enabled
        if is_enabled:
            print(f"{component_to_add} is Enabled")
        elif not is_enabled:
            print(f"{component_to_add} is Disabled. But It should be enabled after adding Mesh")


test = TestMeshSurfaceTagEmitter()
test.run()
