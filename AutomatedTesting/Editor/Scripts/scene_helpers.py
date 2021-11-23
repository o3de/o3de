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