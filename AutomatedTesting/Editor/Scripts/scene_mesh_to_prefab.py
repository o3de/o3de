#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
import os, traceback, binascii, sys, json, pathlib
import azlmbr.math
import azlmbr.bus

#
# SceneAPI Processor
#


def log_exception_traceback():
    exc_type, exc_value, exc_tb = sys.exc_info()
    data = traceback.format_exception(exc_type, exc_value, exc_tb)
    print(str(data))

def get_mesh_node_names(sceneGraph):
    import azlmbr.scene as sceneApi
    import azlmbr.scene.graph
    from scene_api import scene_data as sceneData

    meshDataList = []
    node = sceneGraph.get_root()
    children = []
    paths = []

    while node.IsValid():
        # store children to process after siblings
        if sceneGraph.has_node_child(node):
            children.append(sceneGraph.get_node_child(node))

        nodeName = sceneData.SceneGraphName(sceneGraph.get_node_name(node))
        paths.append(nodeName.get_path())

        # store any node that has mesh data content
        nodeContent = sceneGraph.get_node_content(node)
        if nodeContent.CastWithTypeName('MeshData'):
            if sceneGraph.is_node_end_point(node) is False:
                if (len(nodeName.get_path())):
                    meshDataList.append(sceneData.SceneGraphName(sceneGraph.get_node_name(node)))

        # advance to next node
        if sceneGraph.has_node_sibling(node):
            node = sceneGraph.get_node_sibling(node)
        elif children:
            node = children.pop()
        else:
            node = azlmbr.scene.graph.NodeIndex()

    return meshDataList, paths

def add_material_component(entity_id):
    # Create an override AZ::Render::EditorMaterialComponent
    editor_material_component = azlmbr.entity.EntityUtilityBus(
        azlmbr.bus.Broadcast,
        "GetOrAddComponentByTypeName",
        entity_id,
        "EditorMaterialComponent")

    # this fills out the material asset to a known product AZMaterial asset relative path
    json_update = json.dumps({
            "Controller": { "Configuration": { "materials": [
                {
                    "Key": {},
                    "Value": { "MaterialAsset":{
                        "assetHint": "materials/basic_grey.azmaterial"
                    }}
                }]
            }}
        });
    result = azlmbr.entity.EntityUtilityBus(azlmbr.bus.Broadcast, "UpdateComponentForEntity", entity_id, editor_material_component, json_update)

    if not result:
        raise RuntimeError("UpdateComponentForEntity for editor_material_component failed")

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

    # Loop every mesh node in the scene
    for activeMeshIndex in range(len(mesh_name_list)):
        mesh_name = mesh_name_list[activeMeshIndex]
        mesh_path = mesh_name.get_path()
        # Create a unique mesh group name using the filename + node name
        mesh_group_name = '{}_{}'.format(source_filename_only, mesh_name.get_name())
        # Remove forbidden filename characters from the name since this will become a file on disk later
        mesh_group_name = "".join(char for char in mesh_group_name if char not in "|<>:\"/?*\\")
        # Add the MeshGroup to the manifest and give it a unique ID
        mesh_group = scene_manifest.add_mesh_group(mesh_group_name)
        mesh_group['id'] = '{' + str(uuid.uuid5(uuid.NAMESPACE_DNS, source_filename_only + mesh_path)) + '}'
        # Set our current node as the only node that is included in this MeshGroup
        scene_manifest.mesh_group_select_node(mesh_group, mesh_path)

        # Explicitly remove all other nodes to prevent implicit inclusions
        for node in all_node_paths:
            if node != mesh_path:
                scene_manifest.mesh_group_unselect_node(mesh_group, node)

        # Create an editor entity
        entity_id = azlmbr.entity.EntityUtilityBus(azlmbr.bus.Broadcast, "CreateEditorReadyEntity", mesh_group_name)
        # Add an EditorMeshComponent to the entity
        editor_mesh_component = azlmbr.entity.EntityUtilityBus(azlmbr.bus.Broadcast, "GetOrAddComponentByTypeName", entity_id, "AZ::Render::EditorMeshComponent")
        # Set the ModelAsset assetHint to the relative path of the input asset + the name of the MeshGroup we just created + the azmodel extension
        # The MeshGroup we created will be output as a product in the asset's path named mesh_group_name.azmodel
        # The assetHint will be converted to an AssetId later during prefab loading
        json_update = json.dumps({
            "Controller": { "Configuration": { "ModelAsset": {
                "assetHint": os.path.join(source_relative_path, mesh_group_name) + ".azmodel" }}}
            });
        # Apply the JSON above to the component we created
        result = azlmbr.entity.EntityUtilityBus(azlmbr.bus.Broadcast, "UpdateComponentForEntity", entity_id, editor_mesh_component, json_update)

        if not result:
            raise RuntimeError("UpdateComponentForEntity failed for Mesh component")

        # an example of adding a material component to override the default material
        if previous_entity_id is not None and first_mesh:
            first_mesh = False
            add_material_component(entity_id)

        # Get the transform component
        transform_component = azlmbr.entity.EntityUtilityBus(azlmbr.bus.Broadcast, "GetOrAddComponentByTypeName", entity_id, "27F1E1A1-8D9D-4C3B-BD3A-AFB9762449C0")

        # Set this entity to be a child of the last entity we created
        # This is just an example of how to do parenting and isn't necessarily useful to parent everything like this
        if previous_entity_id is not None:
            transform_json = json.dumps({
                "Parent Entity" : previous_entity_id.to_json()
                });

            # Apply the JSON update
            result = azlmbr.entity.EntityUtilityBus(azlmbr.bus.Broadcast, "UpdateComponentForEntity", entity_id, transform_component, transform_json)

            if not result:
                raise RuntimeError("UpdateComponentForEntity failed for Transform component")

        # Update the last entity id for next time
        previous_entity_id = entity_id

        # Keep track of the entity we set up, we'll add them all to the prefab we're creating later
        created_entities.append(entity_id)

    # Create a prefab with all our entities
    prefab_filename = source_filename_only + ".prefab"
    created_template_id = azlmbr.prefab.PrefabSystemScriptingBus(azlmbr.bus.Broadcast, "CreatePrefab", created_entities, prefab_filename)

    if created_template_id == azlmbr.prefab.InvalidTemplateId:
        raise RuntimeError("CreatePrefab {} failed".format(prefab_filename))

    # Convert the prefab to a JSON string
    output = azlmbr.prefab.PrefabLoaderScriptingBus(azlmbr.bus.Broadcast, "SaveTemplateToString", created_template_id)

    if output.IsSuccess():
        jsonString = output.GetValue()
        uuid = azlmbr.math.Uuid_CreateRandom().ToString()
        jsonResult = json.loads(jsonString)
        # Add a PrefabGroup to the manifest and store the JSON on it
        scene_manifest.add_prefab_group(source_filename_only, uuid, jsonResult)
    else:
        raise RuntimeError("SaveTemplateToString failed for template id {}, prefab {}".format(created_template_id, prefab_filename))

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
