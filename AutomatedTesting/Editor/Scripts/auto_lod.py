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
    import uuid
    import azlmbr.scene as sceneApi
    import azlmbr.scene.graph
    from scene_api import scene_data as sceneData

    graph = sceneData.SceneGraph(scene.graph)
    # Get a list of all the mesh nodes, as well as all the nodes
    mesh_name_list, all_node_paths = get_mesh_node_names(graph)
    mesh_name_list.sort(key=lambda node: str.casefold(node.get_path()))
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

    # Assume the first mesh is the main mesh
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

    # Create a LOD rule
    lod_rule = scene_manifest.mesh_group_add_lod_rule(mesh_group)

    # Loop all the mesh nodes after the first
    for x in mesh_path_list[1:]:
        # Add a new LOD level
        lod = scene_manifest.lod_rule_add_lod(lod_rule)
        # Select the current mesh for this LOD level
        scene_manifest.lod_select_node(lod, x)

        # Unselect every other mesh for this LOD level
        for y in mesh_path_list:
            if y != x:
                scene_manifest.lod_unselect_node(lod, y)

    # Create an editor entity
    entity_id = azlmbr.entity.EntityUtilityBus(azlmbr.bus.Broadcast, "CreateEditorReadyEntity", mesh_group_name)
    # Add an EditorMeshComponent to the entity
    editor_mesh_component = azlmbr.entity.EntityUtilityBus(azlmbr.bus.Broadcast, "GetOrAddComponentByTypeName", entity_id, "AZ::Render::EditorMeshComponent")
    # Set the ModelAsset assetHint to the relative path of the input asset + the name of the MeshGroup we just created + the azmodel extension
    # The MeshGroup we created will be output as a product in the asset's path named mesh_group_name.fbx.azmodel
    # The assetHint will be converted to an AssetId later during prefab loading
    json_update = json.dumps({
        "Controller": { "Configuration": { "ModelAsset": {
            "assetHint": os.path.join(source_relative_path, mesh_group_name) + ".fbx.azmodel" }}}
        });
    # Apply the JSON above to the component we created
    result = azlmbr.entity.EntityUtilityBus(azlmbr.bus.Broadcast, "UpdateComponentForEntity", entity_id, editor_mesh_component, json_update)

    if not result:
        raise RuntimeError("UpdateComponentForEntity failed for Mesh component")

    create_prefab(scene_manifest, source_filename_only, [entity_id])

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
    finally:
        global sceneJobHandler
        # do not delete or set sceneJobHandler to None, just disconnect from it.
        # this call is occuring while the scene Job Handler itself is in the callstack, so deleting it here
        # would cause a crash.
        sceneJobHandler.disconnect()

# try to create SceneAPI handler for processing
try:
    import azlmbr.scene as sceneApi
    sceneJobHandler = sceneApi.ScriptBuildingNotificationBusHandler()
    sceneJobHandler.connect()
    sceneJobHandler.add_callback('OnUpdateManifest', on_update_manifest)
except:
    sceneJobHandler = None
