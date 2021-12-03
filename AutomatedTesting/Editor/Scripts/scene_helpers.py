"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import traceback, logging, json
from typing import Tuple, List

import azlmbr.bus
from scene_api import scene_data as sceneData
from scene_api.scene_data import SceneGraphName


def log_exception_traceback():
    """
    Outputs an exception stacktrace.
    """
    data = traceback.format_exc()
    logger = logging.getLogger('python')
    logger.error(data)


def sanitize_name_for_disk(name: str):
    """
    Removes illegal filename characters from a string.

    :param name: String to clean.
    :return: Name with illegal characters removed.
    """
    return "".join(char for char in name if char not in "|<>:\"/?*\\")


def get_mesh_node_names(scene_graph: sceneData.SceneGraph) -> Tuple[List[SceneGraphName], List[str]]:
    """
    Returns a tuple of all the mesh nodes as well as all the node paths

    :param scene_graph: Scene graph to search
    :return: Tuple of [Mesh Nodes, All Node Paths]
    """
    import azlmbr.scene as sceneApi
    import azlmbr.scene.graph

    mesh_data_list = []
    node = scene_graph.get_root()
    children = []
    paths = []

    while node.IsValid():
        # store children to process after siblings
        if scene_graph.has_node_child(node):
            children.append(scene_graph.get_node_child(node))

        node_name = sceneData.SceneGraphName(scene_graph.get_node_name(node))
        paths.append(node_name.get_path())

        # store any node that has mesh data content
        node_content = scene_graph.get_node_content(node)
        if node_content.CastWithTypeName('MeshData'):
            if scene_graph.is_node_end_point(node) is False:
                if len(node_name.get_path()):
                    mesh_data_list.append(sceneData.SceneGraphName(scene_graph.get_node_name(node)))

        # advance to next node
        if scene_graph.has_node_sibling(node):
            node = scene_graph.get_node_sibling(node)
        elif children:
            node = children.pop()
        else:
            node = azlmbr.scene.graph.NodeIndex()

    return mesh_data_list, paths


def create_prefab(scene_manifest: sceneData.SceneManifest, prefab_name: str, entities: list) -> None:
    prefab_filename = prefab_name + ".prefab"
    created_template_id = azlmbr.prefab.PrefabSystemScriptingBus(azlmbr.bus.Broadcast, "CreatePrefab", entities,
                                                                 prefab_filename)

    if created_template_id is None or created_template_id == azlmbr.prefab.InvalidTemplateId:
        raise RuntimeError("CreatePrefab {} failed".format(prefab_filename))

    # Convert the prefab to a JSON string
    output = azlmbr.prefab.PrefabLoaderScriptingBus(azlmbr.bus.Broadcast, "SaveTemplateToString", created_template_id)

    if output is not None and output.IsSuccess():
        json_string = output.GetValue()
        uuid = azlmbr.math.Uuid_CreateRandom().ToString()
        json_result = json.loads(json_string)
        # Add a PrefabGroup to the manifest and store the JSON on it
        scene_manifest.add_prefab_group(prefab_name, uuid, json_result)
    else:
        raise RuntimeError(
            "SaveTemplateToString failed for template id {}, prefab {}".format(created_template_id, prefab_filename))
