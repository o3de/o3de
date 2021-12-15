#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
import traceback, sys, uuid, os, json, logging
import scene_api.scene_data
import azlmbr.scene.graph

#
# LnBC Custom Properties project script DEC 2021
#

def log_exception_traceback():
    """Outputs an exception stacktrace."""
    data = traceback.format_exc()
    logger = logging.getLogger('python')
    logger.error(data)

def update_manifest(scene):
    scene_graph = scene_api.scene_data.SceneGraph(scene.graph)
    node = scene_graph.get_root()
    children = []

    while node.IsValid():
        if scene_graph.has_node_child(node):
            children.append(scene_graph.get_node_child(node))

        node_name = scene_api.scene_data.SceneGraphName(scene_graph.get_node_name(node))
        node_content = scene_graph.get_node_content(node)
        if node_content.CastWithTypeName('CustomPropertyData'):
            if len(node_name.get_path()):
                print(f'>>> CustomPropertyMap {node_name.get_path()}')
                props = node_content.Invoke('GetPropertyMap', [])
                for index, (key, value) in enumerate(props.items()):
                    print(f'>>>> indxe:{index} key:{key} value:{value}')

        # advance to next node
        if scene_graph.has_node_sibling(node):
            node = scene_graph.get_node_sibling(node)
        elif children:
            node = children.pop()
        else:
            node = azlmbr.scene.graph.NodeIndex()

    sceneManifest = scene_api.scene_data.SceneManifest()
    return sceneManifest.export()

sceneJobHandler = None

def on_update_manifest(args):
    try:
        scene = args[0]
        return update_manifest(scene)
    except RuntimeError as err:
        print (f'ERROR - {err}')
        log_exception_traceback()
    except:
        log_exception_traceback()

    global sceneJobHandler
    sceneJobHandler.disconnect()
    sceneJobHandler = None

# try to create SceneAPI handler for processing
try:
    import azlmbr.scene

    sceneJobHandler = azlmbr.scene.ScriptBuildingNotificationBusHandler()
    sceneJobHandler.connect()
    sceneJobHandler.add_callback('OnUpdateManifest', on_update_manifest)
except:
    sceneJobHandler = None
