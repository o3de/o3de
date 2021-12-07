#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
import traceback, sys, uuid, os, json

import scene_export_utils

#
# Example for exporting MotionGroup scene rules
#

def update_manifest(scene):
    import azlmbr.scene.graph
    import scene_api.scene_data

    graph = scene_api.scene_data.SceneGraph(scene.graph)
    mesh_name_list, all_node_paths = scene_export_utils.get_node_names(graph, 'MeshData')
    scene_manifest = scene_api.scene_data.SceneManifest()

    # Convert the manifest to a JSON string and return it
    return scene_manifest.export()

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
