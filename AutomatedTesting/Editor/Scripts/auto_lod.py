#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
import os, traceback, binascii, sys, json, pathlib, logging
import azlmbr.math
import azlmbr.bus
from scene_helpers import *

#
# SceneAPI Processor
#

def update_manifest(scene):
    import json
    import uuid, os
    import azlmbr.scene as sceneApi
    import azlmbr.scene.graph
    from scene_api import scene_data as sceneData

    graph = sceneData.SceneGraph(scene.graph)
    # Get a list of all the mesh nodes, as well as all the nodes
    mesh_name_list, all_node_paths = get_mesh_node_names(graph)
    scene_manifest = sceneData.SceneManifest()
    
    clean_filename = scene.sourceFilename.replace('.', '_')
    
    # Compute the filename of the scene file
    source_basepath = scene.watchFolder
    source_relative_path = os.path.dirname(os.path.relpath(clean_filename, source_basepath))
    source_filename_only = os.path.basename(clean_filename)

    created_entities = []
    previous_entity_id = azlmbr.entity.InvalidEntityId
    first_mesh = True

    # Make a list of mesh node paths
    mesh_path_list = list(map(lambda node: node.get_path(), mesh_name_list))

    main_mesh = mesh_name_list[0]
    mesh_path = main_mesh.get_path()

    # Create a unique mesh group name using the filename + node name
    mesh_group_name = '{}_{}'.format(source_filename_only, main_mesh.get_name())
    # Remove forbidden filename characters from the name since this will become a file on disk later
    mesh_group_name = "".join(char for char in mesh_group_name if char not in "|<>:\"/?*\\")
    # Add the MeshGroup to the manifest and give it a unique ID
    mesh_group = scene_manifest.add_mesh_group(mesh_group_name)
    mesh_group['id'] = '{' + str(uuid.uuid5(uuid.NAMESPACE_DNS, source_filename_only + mesh_path)) + '}'
    # Set our current node as the only node that is included in this MeshGroup
    scene_manifest.mesh_group_select_node(mesh_group, mesh_path)

    # Explicitly remove all other nodes to prevent implicit inclusions
    for node in mesh_path_list:
        if node != mesh_path:
            scene_manifest.mesh_group_unselect_node(mesh_group, node)

    # Convert the manifest to a JSON string and return it
    new_manifest = scene_manifest.export()

    return new_manifest

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
    sceneJobHandler = None

# try to create SceneAPI handler for processing
try:
    import azlmbr.scene as sceneApi
    if (sceneJobHandler == None):
        sceneJobHandler = sceneApi.ScriptBuildingNotificationBusHandler()
        sceneJobHandler.connect()
        sceneJobHandler.add_callback('OnUpdateManifest', on_update_manifest)
except:
    sceneJobHandler = None
