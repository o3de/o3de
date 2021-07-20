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


class TestLayerExtenderNodeComponentEntitySync(EditorTestHelper):

    def __init__(self):
        EditorTestHelper.__init__(self, log_prefix="LayerExtenderNodeComponentEntitySync", args=["level"])

    def run_test(self):
        """
        Summary:
        This test verifies that all wrapped nodes can be successfully added to/removed from parent nodes.

        Expected Behavior:
        All wrapped extender nodes can be added to/removed from appropriate parent nodes.

        Test Steps:
         1) Create a new level
         2) Open Landscape Canvas and create a new graph
         3) Add Area Blender and Layer Spawner nodes to the graph, and add/remove each extender node to/from each

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

        # Listen for entity creation notifications so we can check if the
        # proper components are added when we add nodes
        handler = editor.EditorEntityContextNotificationBusHandler()
        handler.connect()
        handler.add_callback('OnEditorEntityCreated', onEntityCreated)

        # Extender mapping with the key being the node name and the value is the
        # expected Component that should be added to the layer Entity for that wrapped node
        extenders = {
            'AltitudeFilterNode': 'Vegetation Altitude Filter',
            'DistanceBetweenFilterNode': 'Vegetation Distance Between Filter',
            'DistributionFilterNode': 'Vegetation Distribution Filter',
            'ShapeIntersectionFilterNode': 'Vegetation Shape Intersection Filter',
            'SlopeFilterNode': 'Vegetation Slope Filter',
            'SurfaceMaskDepthFilterNode': 'Vegetation Surface Mask Depth Filter',
            'SurfaceMaskFilterNode': 'Vegetation Surface Mask Filter',
            'PositionModifierNode': 'Vegetation Position Modifier',
            'RotationModifierNode': 'Vegetation Rotation Modifier',
            'ScaleModifierNode': 'Vegetation Scale Modifier',
            'SlopeAlignmentModifierNode': 'Vegetation Slope Alignment Modifier',
            'AssetWeightSelectorNode': 'Vegetation Asset Weight Selector'
        }

        # Retrieve a mapping of the TypeIds for all the components
        # we will be checking for
        componentNames = []
        for name in extenders:
            componentNames.append(extenders[name])
        componentTypeIds = hydra.get_component_type_id_map(componentNames)

        areas = [
            'AreaBlenderNode',
            'SpawnerAreaNode'
        ]

        # Add/remove all our supported extender nodes to the Layer Areas and check if the appropriate
        # Components are added/removed to the wrapper node's Entity
        newGraph = graph.GraphManagerRequestBus(bus.Broadcast, 'GetGraph', newGraphId)
        x = 10.0
        y = 10.0
        for areaName in areas:
            nodePosition = math.Vector2(x, y)
            areaNode = landscapecanvas.LandscapeCanvasNodeFactoryRequestBus(bus.Broadcast, 'CreateNodeForTypeName',
                                                                            newGraph, areaName)
            graph.GraphControllerRequestBus(bus.Event, 'AddNode', newGraphId, areaNode, nodePosition)

            success = True
            for extenderName in extenders:
                # Add the wrapped node for the extender
                extenderNode = landscapecanvas.LandscapeCanvasNodeFactoryRequestBus(bus.Broadcast,
                                                                                    'CreateNodeForTypeName', newGraph,
                                                                                    extenderName)
                graph.GraphControllerRequestBus(bus.Event, 'AddNode', newGraphId, extenderNode, nodePosition)
                graph.GraphControllerRequestBus(bus.Event, 'WrapNode', newGraphId, areaNode, extenderNode)

                # Check that the appropriate Component was added when the extender node was added
                extenderComponent = extenders[extenderName]
                componentTypeId = componentTypeIds[extenderComponent]
                success = success and editor.EditorComponentAPIBus(bus.Broadcast, 'HasComponentOfType', newEntityId,
                                                                   componentTypeId)
                self.test_success = self.test_success and success
                if not success:
                    self.log("{node} failed to add {component} Component".format(node=areaName,
                                                                              component=extenderComponent))
                    break

                # Check that the appropriate Component was removed when the extender node was removed
                graph.GraphControllerRequestBus(bus.Event, 'RemoveNode', newGraphId, extenderNode)
                success = success and not editor.EditorComponentAPIBus(bus.Broadcast, 'HasComponentOfType', newEntityId,
                                                                       componentTypeId)
                self.test_success = self.test_success and success
                if not success:
                    self.log("{node} failed to remove {component} Component".format(node=areaName,
                                                                                 component=extenderComponent))
                    break

            if success:
                self.log("{node} successfully added and removed all filters/modifiers/selectors".format(node=areaName))

        # Stop listening for entity creation notifications
        handler.disconnect()


test = TestLayerExtenderNodeComponentEntitySync()
test.run()
