import traceback, logging, json
import azlmbr.bus

def log_exception_traceback():
    data = traceback.format_exc()
    logger = logging.getLogger('python')
    logger.error(data)

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
    
def create_prefab(scene_manifest, prefab_name, entities):
    prefab_filename = prefab_name + ".prefab"
    created_template_id = azlmbr.prefab.PrefabSystemScriptingBus(azlmbr.bus.Broadcast, "CreatePrefab", entities, prefab_filename)

    if created_template_id is None or created_template_id == azlmbr.prefab.InvalidTemplateId:
        raise RuntimeError("CreatePrefab {} failed".format(prefab_filename))

    # Convert the prefab to a JSON string
    output = azlmbr.prefab.PrefabLoaderScriptingBus(azlmbr.bus.Broadcast, "SaveTemplateToString", created_template_id)

    if output is not None and output.IsSuccess():
        jsonString = output.GetValue()
        uuid = azlmbr.math.Uuid_CreateRandom().ToString()
        jsonResult = json.loads(jsonString)
        # Add a PrefabGroup to the manifest and store the JSON on it
        scene_manifest.add_prefab_group(prefab_name, uuid, jsonResult)
    else:
        raise RuntimeError("SaveTemplateToString failed for template id {}, prefab {}".format(created_template_id, prefab_filename))