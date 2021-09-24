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

def raise_error(message):
    print (f'ERROR - {message}');
    raise RuntimeError(f'[ERROR]: {message}');

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

def update_manifest(scene):
    import json
    import uuid, os
    import azlmbr.scene as sceneApi
    import azlmbr.scene.graph
    from scene_api import scene_data as sceneData

    graph = sceneData.SceneGraph(scene.graph)
    mesh_name_list, all_node_paths = get_mesh_node_names(graph)
    scene_manifest = sceneData.SceneManifest()
    source_basepath = scene.watchFolder

    clean_filename = scene.sourceFilename.replace('.', '_')
    
    source_relative_path = os.path.dirname(os.path.relpath(clean_filename, source_basepath))
    source_filename_only = os.path.basename(clean_filename)

    created_entities = []

    for activeMeshIndex in range(len(mesh_name_list)):
        mesh_name = mesh_name_list[activeMeshIndex]
        mesh_path = mesh_name.get_path()
        mesh_group_name = '{}_{}'.format(source_filename_only, mesh_name.get_name()).replace('|', '_')
        mesh_group = scene_manifest.add_mesh_group(mesh_group_name)
        mesh_group['id'] = '{' + str(uuid.uuid5(uuid.NAMESPACE_DNS, source_filename_only + mesh_path)) + '}'
        scene_manifest.mesh_group_select_node(mesh_group, mesh_path)

        for node in all_node_paths:
            if node != mesh_path:
                scene_manifest.mesh_group_unselect_node(mesh_group, node)

        entity_id = azlmbr.entity.EntityUtilityBus(azlmbr.bus.Broadcast, "CreateEditorReadyEntity", mesh_group_name)
        editor_mesh_component = azlmbr.entity.EntityUtilityBus(azlmbr.bus.Broadcast, "GetOrAddComponentByTypeName", entity_id, "AZ::Render::EditorMeshComponent")
        json_update = json.dumps({
                            "Controller": { "Configuration": { "ModelAsset": {
                                "assetHint": os.path.join(source_relative_path, mesh_group_name) + ".azmodel" }}}
                            });
        result = azlmbr.entity.EntityUtilityBus(azlmbr.bus.Broadcast, "UpdateComponentForEntity", entity_id, editor_mesh_component, json_update)

        if not result:
            raise_error("UpdateComponentForEntity failed")
            return

        created_entities.append(entity_id)

    my_template = azlmbr.prefab.PrefabSystemScriptingBus(azlmbr.bus.Broadcast, "CreatePrefab", created_entities, source_filename_only + ".prefab")

    if my_template == azlmbr.prefab.InvalidTemplateId:
        raise_error("CreatePrefab failed")
        return

    output = azlmbr.prefab.PrefabLoaderScriptingBus(azlmbr.bus.Broadcast, "SaveTemplateToString", my_template)

    if output.IsSuccess():
        jsonString = output.GetValue()
        uuid = azlmbr.math.Uuid_CreateRandom().ToString()
        jsonResult = json.loads(jsonString)
        scene_manifest.add_prefab_group(source_filename_only, uuid, jsonResult)
    else:
        raise_error("SaveTemplateToString failed")

    new_manifest = scene_manifest.export()

    return new_manifest

sceneJobHandler = None

def on_update_manifest(args):
    try:
        scene = args[0]
        return update_manifest(scene)
    except:
        global sceneJobHandler
        sceneJobHandler = None
        log_exception_traceback()

# try to create SceneAPI handler for processing
try:
    import azlmbr.scene as sceneApi
    if (sceneJobHandler == None):
        sceneJobHandler = sceneApi.ScriptBuildingNotificationBusHandler()
        sceneJobHandler.connect()
        sceneJobHandler.add_callback('OnUpdateManifest', on_update_manifest)
except:
    sceneJobHandler = None
