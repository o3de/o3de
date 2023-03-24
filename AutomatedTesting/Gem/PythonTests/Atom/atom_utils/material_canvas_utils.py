"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import azlmbr.atomtools as atomtools
import azlmbr.editor.graph as graph
import azlmbr.math as math
import azlmbr.bus as bus

import Atom.atom_utils.atom_tools_utils as atom_tools_utils
from Atom.atom_utils.atom_constants import (
    DynamicNodeManagerRequestBusEvents, GraphControllerRequestBusEvents, GraphDocumentRequestBusEvents)


class Graph(object):
    """
    Opens the specified materialgraph file when inside MaterialCanvas which appears as a Graph.
    Opening the Graph is required since it also sets the document_id for use with other functions.
    """

    def __init__(self, material_graph_file_path: str):
        self.material_graph_file_path = material_graph_file_path
        self.document_id = atom_tools_utils.open_document(self.material_graph_file_path)
        self.graph_id = atomtools.GraphDocumentRequestBus(
            bus.Event, GraphDocumentRequestBusEvents.GET_GRAPH_ID, self.document_id)
        self.name = atomtools.GraphDocumentRequestBus(
            bus.Event, GraphDocumentRequestBusEvents.GET_GRAPH_NAME, self.document_id)
        self.graph_object = atomtools.GraphDocumentRequestBus(
            bus.Event, GraphDocumentRequestBusEvents.GET_GRAPH, self.document_id)
        self.nodes = []

    def _create_node_by_name(self, node_name: str) -> object:
        """
        Creates a new node in memory matching the node_name string on the specified graph object.
        i.e. "World Position" for node_name would create a World Position node.
        :param node_name: String representing the type of node to add to the graph.
        :return: azlmbr.object.PythonProxyObject representing a C++ AZStd::shared_ptr<Node> object.
        """
        return atomtools.DynamicNodeManagerRequestBus(
            bus.Broadcast, DynamicNodeManagerRequestBusEvents.CREATE_NODE_BY_NAME, self.graph_object, node_name)

    def add_node(self, node_name: str, position: math.Vector2) -> None:
        """
        Creates a node in memory using _create_node_by_name then adds it to this Graph object.
        :param node_name: String representing the type of node to add to the graph.
        :param position: math.Vector2(x,y) value that determines where to place the node on the Graph's grid.
        :return: None, but appends the new node to self.nodes list.
        """
        node_object = self._create_node_by_name(node_name)
        node_id = graph.GraphControllerRequestBus(
            bus.Event, GraphControllerRequestBusEvents.ADD_NODE, self.graph_id, node_object, position)
        self.nodes.append(Node(node_object, node_id, node_name, self.graph_id, position))


class Node(object):
    """
    Represents a node inside of a Graph (required).
    """

    def __init__(self, node_object: object, node_id: int, node_name: str, graph_id: math.Uuid, position: math.Vector2):
        self.node_object = node_object
        self.node_id = node_id
        self.node_name = node_name
        self.graph_id = graph_id
        self.position = position
        self.outbound_slot = graph.GraphModelSlotId('outPosition')
        self.inbound_slot = graph.GraphModelSlotId('inPositionOffset')

    def connect_to_node_outbound_slot(self, target_node: Node) -> object:
        """
        Adds a new connection between this node's inbound slot to target_node's outbound slot.
        :param target_node: A Node() object for the target node to connect to.
        :return: azlmbr.object.PythonProxyObject representing a C++ AZStd::shared_ptr<Connection> object.
        """
        return graph.GraphControllerRequestBus(
            bus.Event, GraphControllerRequestBusEvents.ADD_CONNECTION_BY_SLOT_ID, self.graph_id,
            self.node_object, self.inbound_slot, target_node, target_node.outbound_slot)

    def connect_to_node_inbound_slot(self, target_node: Node) -> object:
        """
        Adds a new connection between this node's outbound slot to target_node's inbound slot.
        :param target_node: A Node() object for the target node to connect to.
        :return: azlmbr.object.PythonProxyObject representing a C++ AZStd::shared_ptr<Connection> object.
        """
        return graph.GraphControllerRequestBus(
            bus.Event, GraphControllerRequestBusEvents.ADD_CONNECTION_BY_SLOT_ID, self.graph_id,
            self.node_object, self.outbound_slot, target_node, target_node.inbound_slot)

    def is_connected_to_node_outbound_slot(self, target_node: Node) -> bool:
        """
        Determines if self.node_object's inbound slot is connected to the target_node's outbound slot.
        :param target_node: A Node() object for the target node to connect to.
        :return: bool representing success (True) or failure (False)
        """
        return graph.GraphControllerRequestBus(
            bus.Event, GraphControllerRequestBusEvents.ARE_SLOTS_CONNECTED, self.graph_id,
            self.node_object, self.inbound_slot, target_node, target_node.outbound_slot)

    def is_connected_to_node_inbound_slot(self, target_node: Node) -> bool:
        """
        Determines if self.node_object's outbound slot is connected to the target_node's inbound slot.
        :param target_node: A Node() object for the target node to connect to.
        :return: bool representing success (True) or failure (False)
        """
        return graph.GraphControllerRequestBus(
            bus.Event, GraphControllerRequestBusEvents.ARE_SLOTS_CONNECTED, self.graph_id,
            self.node_object, self.outbound_slot, target_node, target_node.inbound_slot)
