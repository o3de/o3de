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

from Atom.atom_utils.atom_constants import (
    DynamicNodeManagerRequestBusEvents, GraphControllerRequestBusEvents, GraphDocumentRequestBusEvents)


class Node(object):
    """
    Represents a node inside of a graph and contains node related functions.
    """

    def __init__(self, node_python_proxy: azlmbr.object.PythonProxyObject):
        """
        :param node_python_proxy: azlmbr.object.PythonProxyObject representing a node object.
        """
        self.node_python_proxy = node_python_proxy

    def get_slots(self) -> dict:
        """
        Returns a dictionary mapping of all slots on this Node.
        :return: a dict containing slot name keys with graph.GraphModelSlotId slot values.
        """
        mapped_node_slots = {}
        raw_node_slots_dict = self.node_python_proxy.invoke('GetSlots')

        for k, v in raw_node_slots_dict.items():
            mapped_node_slots[k.name] = graph.GraphModelSlotId(k.name)

        return mapped_node_slots


class Graph(object):
    """
    Binds this Graph object to the opened graph matching the document_id passed to construct this object.
    """

    def __init__(self, document_id: math.Uuid):
        self.document_id = document_id

    def get_graph_id(self) -> math.Uuid:
        """
        Returns the graph ID value for this Graph object.
        :return: math.Uuid which represents the graph ID value.
        """
        return atomtools.GraphDocumentRequestBus(
            bus.Event, GraphDocumentRequestBusEvents.GET_GRAPH_ID, self.document_id)

    def get_graph_name(self) -> str:
        """
        Returns the graph name for this Graph object.
        :return: string representing the name of this Graph object.
        """
        return atomtools.GraphDocumentRequestBus(
            bus.Event, GraphDocumentRequestBusEvents.GET_GRAPH_NAME, self.document_id)

    def get_graph(self) -> azlmbr.object.PythonProxyObject:
        """
        Returns the AZStd::shared_ptr<Graph> object for this Graph object.
        :return: azlmbr.object.PythonProxyObject containing a AZStd::shared_ptr<Graph> object.
        """
        return atomtools.GraphDocumentRequestBus(bus.Event, GraphDocumentRequestBusEvents.GET_GRAPH, self.document_id)

    def get_nodes(self) -> list[azlmbr.object.PythonProxyObject]:
        """
        Gets the current Nodes in this Graph and returns them as a list of azlmbr.object.PythonProxyObject objects.
        :return: list of azlmbr.object.PythonProxyObject objects each representing a Node in this Graph.
        """
        return graph.GraphControllerRequestBus(
            bus.Event, GraphControllerRequestBusEvents.GET_NODES, self.get_graph_id())

    def create_node_by_name(self, node_name: str) -> azlmbr.object.PythonProxyObject:
        """
        Creates a new node in memory matching the node_name string on the specified graph object.
        i.e. "World Position" for node_name would create a World Position node.
        We create them in memory for use with the GraphControllerRequestBusEvents.ADD_NODE bus call on this Graph.
        :param node_name: String representing the type of node to add to the graph.
        :return: azlmbr.object.PythonProxyObject representing a C++ AZStd::shared_ptr<Node> object.
        """
        return atomtools.DynamicNodeManagerRequestBus(
            bus.Broadcast, DynamicNodeManagerRequestBusEvents.CREATE_NODE_BY_NAME, self.get_graph(), node_name)

    def add_node(self, node: azlmbr.object.PythonProxyObject, position: math.Vector2) -> Node:
        """
        Adds a node generated from the NodeManager to this Graph, then returns a Node object for that node.
        :param node: A azlmbr.object.PythonProxyObject containing the node we wish to add to the graph.
        :param position: math.Vector2(x,y) value that determines where to place the node on the Graph's grid.
        :return: A Node object containing node object and node id via azlmbr.object.PythonProxyObject objects.
        """
        graph.GraphControllerRequestBus(
            bus.Event, GraphControllerRequestBusEvents.ADD_NODE, self.get_graph_id(), node, position)
        return Node(node)

    def add_connection_by_slot_id(self,
                                  source_node: Node,
                                  source_slot: graph.GraphModelSlotId,
                                  target_node: Node,
                                  target_slot: graph.GraphModelSlotId) -> azlmbr.object.PythonProxyObject:
        """
        Connects a starting source_slot to a destination target_slot.
        :param source_node: A Node() object for the source node we are connecting from.
        :param source_slot: A graph.GraphModelSlotId representing a slot on the source_node.
        :param target_node: A Node() object for the target node we are connecting to.
        :param target_slot: A graph.GraphModelSlotId representing a slot on the target_node.
        :return: azlmbr.object.PythonProxyObject representing a C++ AZStd::shared_ptr<Connection> object.
        """
        return graph.GraphControllerRequestBus(
            bus.Event, GraphControllerRequestBusEvents.ADD_CONNECTION_BY_SLOT_ID, self.get_graph_id(),
            source_node.node_python_proxy, source_slot, target_node.node_python_proxy, target_slot)

    def are_slots_connected(self,
                            source_node: Node,
                            source_slot: graph.GraphModelSlotId,
                            target_node: Node,
                            target_slot: graph.GraphModelSlotId) -> bool:
        """
        Determines if the source_slot is connected to the target_slot on the target_node.
        :param source_node: A Node() object for the source node that should be connected to the target node.
        :param source_slot: A graph.GraphModelSlotId representing a slot on the source_node.
        :param target_node: A Node() object for the target node that should be connected to the source node.
        :param target_slot: A graph.GraphModelSlotId representing a slot on the target_node..
        :return: True if the source_slot and target_slot are connected, False otherwise.
        """
        return graph.GraphControllerRequestBus(
            bus.Event, GraphControllerRequestBusEvents.ARE_SLOTS_CONNECTED, self.get_graph_id(),
            source_node.node_python_proxy, source_slot, target_node.node_python_proxy, target_slot)
