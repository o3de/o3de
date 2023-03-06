"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import azlmbr.atom
import azlmbr.atomtools
import azlmbr.atomtools as atomtools
import azlmbr.bus
import azlmbr.bus as bus
import azlmbr.editor.graph as graph
import azlmbr.math as math
import azlmbr.paths
import azlmbr.std
import collections
import os
import pathlib
import random
import sys
import time

def main():
    test_document_id = atomtools.AtomToolsDocumentSystemRequestBus(bus.Broadcast, "CreateDocumentFromTypeName", "Material Graph")
    print(f"{test_document_id.ToString()}")

    test_graph = atomtools.GraphDocumentRequestBus(bus.Event, "GetGraph", test_document_id)
    print(f"{test_graph=}")
    print(f"{test_graph.invoke('GetSystemName')}")

    test_graph_id = atomtools.GraphDocumentRequestBus(bus.Event, "GetGraphId", test_document_id)
    print(f"{test_graph_id.ToString()}")

    test_output_node = atomtools.DynamicNodeManagerRequestBus(bus.Broadcast, "CreateNodeByName", test_graph, "Standard PBR")
    print(f"{test_output_node=}")
    print(f"{test_output_node.typename}")
    print(f"{test_output_node.invoke('GetId')}")

    graph.GraphControllerRequestBus(bus.Event, "AddNode", test_graph_id, test_output_node, math.Vector2(0.0, 0.0))
    test_output_node_id = graph.GraphControllerRequestBus(bus.Event, "GetNodeIdByNode", test_graph_id, test_output_node)
    print(f"{test_output_node_id.ToString()}")


if __name__ == "__main__":
    main()
