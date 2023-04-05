"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import azlmbr.atomtools as atomtools
import azlmbr.editor.graph as graph
import azlmbr.math as math
import azlmbr.object
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
        if not node_id:
            return  # If the node isn't valid, don't add it.
        self.nodes.append(Node(node_object, node_id, node_name, self.graph_id, position))


class Slot(object):
    """
    Represents a slot that is part of a Node (required).
    """

    def __init__(self, slot_name):
        self.slot_name = slot_name
        self.id = graph.GraphModelSlotId(slot_name)
        if not self.id:
            self.id = None


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
        self.slots = []

    def add_slot(self, slot_name: str) -> None:
        """
        Adds a new Slot object to this Node to identify one of its existing slots.
        :param slot_name: Name of the slot to lookup using graph.GraphModelSlotId().
        :return: None, but adds the new slot to the self.slots list.
        """
        self.slots.append(Slot(slot_name))

    def connect_slots(self, source_slot: Slot, target_node: (), target_slot: Slot) -> azlmbr.object.PythonProxyObject:
        """
        Connects a starting source_slot to a destination target_slot.
        :param source_slot: A Slot() object for the source_node in a given Node() object.
        :param target_node: A Node() object for the target node that holds the target_slot we are connecting to.
        :param target_slot: A Slot() object for the slot we are connecting to from the source_node.
        :return: azlmbr.object.PythonProxyObject representing a C++ AZStd::shared_ptr<Connection> object.
        """
        return graph.GraphControllerRequestBus(
            bus.Event, GraphControllerRequestBusEvents.ADD_CONNECTION_BY_SLOT_ID, self.graph_id,
            self.node_object, source_slot.id, target_node.node_object, target_slot.id)

    def are_slots_connected(self, source_slot: Slot, target_node: (), target_slot: Slot) -> bool:
        """
        Determines if the source_slot is connected to the target_slot on the target_node.
        :param source_slot: A Slot() object for the source_node in a given Node() object.
        :param target_node: A Node() object for the target node that should be connected to the source node.
        :param target_slot: A Slot() object for the slot we should be connected to from the source_node.
        :return: True if the source_slot and target_slot are connected, False otherwise.
        """
        return graph.GraphControllerRequestBus(
            bus.Event, GraphControllerRequestBusEvents.ARE_SLOTS_CONNECTED, self.graph_id,
            self.node_object, source_slot.id, target_node.node_object, target_slot.id)
