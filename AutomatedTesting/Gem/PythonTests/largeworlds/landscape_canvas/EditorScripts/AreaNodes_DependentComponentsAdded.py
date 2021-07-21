"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import os
import sys

import azlmbr.bus as bus
import azlmbr.editor as editor
import azlmbr.editor.graph as graph
import azlmbr.landscapecanvas as landscapecanvas
import azlmbr.legacy.general as general
import azlmbr.math as math
import azlmbr.paths

sys.path.append(os.path.join(azlmbr.paths.devroot, 'AutomatedTesting', 'Gem', 'PythonTests'))
import editor_python_test_tools.hydra_editor_utils as hydra
from editor_python_test_tools.editor_test_helper import EditorTestHelper

editorId = azlmbr.globals.property.LANDSCAPE_CANVAS_EDITOR_ID
newEntityId = None


class TestAreaNodeComponentDependency(EditorTestHelper):
    def __init__(self):
        EditorTestHelper.__init__(self, log_prefix="AreaNodeComponentDependency", args=["level"])

    def run_test(self):
        """
        Summary:
        This test verifies that the Landscape Canvas nodes can be added to a graph, and correctly create entities with
        proper dependent components.

        Expected Behavior:
        All expected component dependencies are met when adding an area node to a graph.

        Test Steps:
         1) Create a new level
         2) Open Landscape Canvas and create a new graph
         3) Drag each of the area nodes to the graph area, and ensure the proper dependent components are added

        Note:
        - This test file must be called from the Open 3D Engine Editor command terminal
        - Any passed and failed tests are written to the Editor.log file.
                Parsing the file or running a log_monitor are required to observe the test results.

        :return: None
        """

        def onEntityCreated(parameters):
            global newEntityId
            newEntityId = parameters[0]

        # Create a new empty level
        self.test_success = self.create_level(
            self.args["level"],
            heightmap_resolution=128,
            heightmap_meters_per_pixel=1,
            terrain_texture_resolution=128,
            use_terrain=False,
        )

        # Open Landscape Canvas tool and verify
        general.open_pane('Landscape Canvas')
        self.test_success = self.test_success and general.is_pane_visible('Landscape Canvas')
        if general.is_pane_visible('Landscape Canvas'):
            self.log('Landscape Canvas pane is open')

        # Create a new graph in Landscape Canvas
        newGraphId = graph.AssetEditorRequestBus(bus.Event, 'CreateNewGraph', editorId)
        self.test_success = self.test_success and newGraphId
        if newGraphId:
            self.log("New graph created")

        # Make sure the graph we created is in Landscape Canvas
        success = graph.AssetEditorRequestBus(bus.Event, 'ContainsGraph', editorId, newGraphId)
        self.test_success = self.test_success and success
        if success:
            self.log("Graph registered with Landscape Canvas")

        # Listen for entity creation notifications so we can check if the entity created
        # from adding these vegetation area nodes has the main target Vegetation Layer Component
        # as well as automatically adding all required dependency components
        handler = editor.EditorEntityContextNotificationBusHandler()
        handler.connect()
        handler.add_callback('OnEditorEntityCreated', onEntityCreated)

        # Vegetation area mapping with the key being the node name and the value is the
        # expected Components that should be added to the Entity created for the node
        areas = {
            'SpawnerAreaNode': [
                'Vegetation Layer Spawner',
                'Vegetation Asset List',
                'Vegetation Reference Shape'
            ],
            'MeshBlockerAreaNode': [
                'Vegetation Layer Blocker (Mesh)',
                'Mesh'
            ],
            'BlockerAreaNode': [
                'Vegetation Layer Blocker',
                'Vegetation Reference Shape'
            ]
        }

        # Retrieve a mapping of the TypeIds for all the components
        # we will be checking for
        componentNames = []
        for name in areas:
            componentNames.extend(areas[name])
        componentTypeIds = hydra.get_component_type_id_map(componentNames)

        # Create nodes for the vegetation areas that have additional required dependencies and check if
        # the Entity created by adding the node has the appropriate component and required
        # additional components added automatically to it
        newGraph = graph.GraphManagerRequestBus(bus.Broadcast, 'GetGraph', newGraphId)
        x = 10.0
        y = 10.0
        for nodeName in areas:
            nodePosition = math.Vector2(x, y)
            node = landscapecanvas.LandscapeCanvasNodeFactoryRequestBus(bus.Broadcast, 'CreateNodeForTypeName',
                                                                        newGraph, nodeName)
            graph.GraphControllerRequestBus(bus.Event, 'AddNode', newGraphId, node, nodePosition)

            components = areas[nodeName]
            success = False
            for component in components:
                componentTypeId = componentTypeIds[component]
                success = editor.EditorComponentAPIBus(bus.Broadcast, 'HasComponentOfType', newEntityId,
                                                       componentTypeId)
                if not success:
                    break
            self.test_success = self.test_success and success
            if success:
                self.log("{node} created new Entity with all required components".format(node=nodeName))

            x += 40.0
            y += 40.0

        # Stop listening for entity creation notifications
        handler.disconnect()


test = TestAreaNodeComponentDependency()
test.run()
