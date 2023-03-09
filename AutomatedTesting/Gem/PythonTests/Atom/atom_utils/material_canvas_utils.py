"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import azlmbr.atomtools as atomtools
import azlmbr.editor.graph as graph
import azlmbr.math as math
import azlmbr.bus as bus

from Atom.atom_utils.atom_constants import (
    DynamicNodeManagerRequestBusEvents, GraphControllerRequestBusEvents, GraphDocumentRequestBusEvents)


def get_graph_name(document_id: math.Uuid) -> str:
    """
    Gets the graph name of the given document_id and returns it as a string.
    :param document_id: The UUID of a given graph document file.
    :return: str representing the graph name contained in document_id
    """
    return atomtools.GraphDocumentRequestBus(bus.Event, GraphDocumentRequestBusEvents.GET_GRAPH_NAME, document_id)


def get_graph_id(document_id: math.Uuid) -> int:
    """
    Gets the graph ID of the given document_id and returns it as an int.
    :param document_id: The UUID of a given graph document file.
    :return: int representing the graph ID of the graph contained in document_id
    """
    return atomtools.GraphDocumentRequestBus(bus.Event, GraphDocumentRequestBusEvents.GET_GRAPH_ID, document_id)


def get_graph(document_id: math.Uuid) -> object:
    """
    Gets the graph object of the given document_id and returns it.
    :param document_id: The UUID of a given graph document file.
    :return: azlmbr.object.PythonProxyObject representing a C++ AZStd::shared_ptr<Graph> object.
    """
    return atomtools.GraphDocumentRequestBus(bus.Event, GraphDocumentRequestBusEvents.GET_GRAPH, document_id)


def create_node_by_name(graph: object, node_name: str) -> object:
    """
    Creates a new node in memory matching the node_name string on the specified graph object.
    i.e. "World Position" for node_name would create a World Position node.
    :param graph: An AZStd::shared_ptr<Graph> graph object to create the new node on.
    :param node_name: String representing the type of node to add to the graph.
    :return: azlmbr.object.PythonProxyObject representing a C++ AZStd::shared_ptr<Node> object.
    """
    return atomtools.DynamicNodeManagerRequestBus(
        bus.Broadcast, DynamicNodeManagerRequestBusEvents.CREATE_NODE_BY_NAME, graph, node_name)


def add_node(graph_id: math.Uuid, node: object, position: math.Vector2) -> int:
    """
    Adds a node saved in memory to the current graph document at a specific position on the graph grid.
    :param graph_id: int representing the ID value of a given graph AZStd::shared_ptr<Graph> object.
    :param node: An AZStd::shared_ptr<Node> object.
    :param position: math.Vector2(x,y) value that determines where to place the node on the graph grid.
    :return: int representing the node ID of the newly placed node.
    """
    return graph.GraphControllerRequestBus(
        bus.Event, GraphControllerRequestBusEvents.ADD_NODE, graph_id, node, position)


def get_graph_model_slot_id(slot_name: str) -> object:
    """
    Given a slot_name string, return a GraphModelSlotId object representing a node slot.
    :param slot_name: String representing the name of the slot to target on the node.
    :return: An GraphModelSlotId object.
    """
    return graph.GraphModelSlotId(slot_name)


def add_connection_by_slot_id(
        graph_id: math.Uuid,
        source_node: object, source_slot: object,
        target_node: object, target_slot: object) -> object:
    """
    Adds a new connection between a source node slot and a target node slot.
    :param graph_id: int representing the ID value of a given graph AZStd::shared_ptr<Graph> object.
    :param source_node: A proxy AZStd::shared_ptr<Node> object for the source node to start the connection from.
    :param source_slot: A proxy GraphModelSlotId object for the slot on the source node to use for the connection.
    :param target_node: A proxy AZStd::shared_ptr<Node> object for the target node to end the connection to.
    :param target_slot: A proxy GraphModelSlotId object for the slot on the target node to use for the connection.
    :return: azlmbr.object.PythonProxyObject representing a C++ AZStd::shared_ptr<Connection> object.
    """
    return graph.GraphControllerRequestBus(
        bus.Event, GraphControllerRequestBusEvents.ADD_CONNECTION_BY_SLOT_ID, graph_id,
        source_node, source_slot, target_node, target_slot)


def are_slots_connected(
        graph_id: math.Uuid,
        source_node: object, source_slot: object,
        target_node: object, target_slot: object) -> bool:
    """
    Determines if 2 nodes have a connection formed between them and returns a boolean for success/failure.
    :param graph_id: int representing the ID value of a given graph AZStd::shared_ptr<Graph> object.
    :param source_node: An AZStd::shared_ptr<Node> object representing the source node for the connection.
    :param source_slot: An GraphModelSlotId object representing the slot on the source node the connection uses.
    :param target_node: An AZStd::shared_ptr<Node> object representing the target node for the connection.
    :param target_slot: An GraphModelSlotId object representing the slot on the target node the connection uses.
    :return: bool representing success (True) or failure (False).
    """
    return graph.GraphControllerRequestBus(
        bus.Event, GraphControllerRequestBusEvents.ARE_SLOTS_CONNECTED, graph_id,
        source_node, source_slot, target_node, target_slot)
