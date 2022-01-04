#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
import traceback, sys, uuid, os, json

#
# Utility methods for processing scenes
#

def log_exception_traceback():
    exc_type, exc_value, exc_tb = sys.exc_info()
    data = traceback.format_exception(exc_type, exc_value, exc_tb)
    print(str(data))

def get_node_names(sceneGraph, nodeTypeName, testEndPoint = False, validList = None):
    import azlmbr.scene.graph
    import scene_api.scene_data

    node = sceneGraph.get_root()
    nodeList = []
    children = []
    paths = []

    while node.IsValid():
        # store children to process after siblings
        if sceneGraph.has_node_child(node):
            children.append(sceneGraph.get_node_child(node))

        nodeName = scene_api.scene_data.SceneGraphName(sceneGraph.get_node_name(node))
        paths.append(nodeName.get_path())
        
        include = True 
        
        if (validList is not None):
            include = False # if a valid list filter provided, assume to not include node name
            name_parts = nodeName.get_path().split('.')
            for valid in validList:
                if (valid in name_parts[-1]):
                    include = True
                    break

        # store any node that has provides specifc data content
        nodeContent = sceneGraph.get_node_content(node)
        if include and nodeContent.CastWithTypeName(nodeTypeName):
            if testEndPoint is not None:
                include = sceneGraph.is_node_end_point(node) is testEndPoint
            if include:
                if (len(nodeName.get_path())):
                    nodeList.append(scene_api.scene_data.SceneGraphName(sceneGraph.get_node_name(node)))

        # advance to next node
        if sceneGraph.has_node_sibling(node):
            node = sceneGraph.get_node_sibling(node)
        elif children:
            node = children.pop()
        else:
            node = azlmbr.scene.graph.NodeIndex()

    return nodeList, paths
