"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
import azlmbr.scene as sceneApi
import json

# Wraps the AZ.SceneAPI.Containers.SceneGraph.NodeIndex internal class
class SceneGraphNodeIndex:
    def __init__(self, sceneGraphNodeIndex) -> None:
        self.nodeIndex = sceneGraphNodeIndex

    def as_number(self):
        return self.nodeIndex.AsNumber()

    def distance(self, other):
        return self.nodeIndex.Distance(other)

    def is_valid(self) -> bool:
        return self.nodeIndex.IsValid()

    def equal(self, other) -> bool:
        return self.nodeIndex.Equal(other)

# Wraps AZ.SceneAPI.Containers.SceneGraph.Name internal class
class SceneGraphName():
    def __init__(self, sceneGraphName) -> None:
        self.name = sceneGraphName

    def get_path(self) -> str:
        return self.name.GetPath()

    def get_name(self) -> str:
        return self.name.GetName()

# Wraps AZ.SceneAPI.Containers.SceneGraph class
class SceneGraph():
    def __init__(self, sceneGraphInstance) -> None:
        self.sceneGraph = sceneGraphInstance

    @classmethod
    def is_valid_name(cls, name):
        return sceneApi.SceneGraph_IsValidName(name)

    @classmethod
    def node_seperation_character(cls):
        return sceneApi.SceneGraph_GetNodeSeperationCharacter()

    def get_node_name(self, node):
        return self.sceneGraph.GetNodeName(node)

    def get_root(self):
        return self.sceneGraph.GetRoot()

    def has_node_content(self, node) -> bool:
        return self.sceneGraph.HasNodeContent(node)

    def has_node_sibling(self, node) -> bool:
        return self.sceneGraph.HasNodeSibling(node)

    def has_node_child(self, node) -> bool:
        return self.sceneGraph.HasNodeChild(node)

    def has_node_parent(self, node) -> bool:
        return self.sceneGraph.HasNodeParent(node)

    def is_node_end_point(self, node) -> bool:
        return self.sceneGraph.IsNodeEndPoint(node)

    def get_node_parent(self, node):
        return self.sceneGraph.GetNodeParent(node)

    def get_node_sibling(self, node):
        return self.sceneGraph.GetNodeSibling(node)

    def get_node_child(self, node):
        return self.sceneGraph.GetNodeChild(node)

    def get_node_count(self):
        return self.sceneGraph.GetNodeCount()

    def find_with_path(self, path):
        return self.sceneGraph.FindWithPath(path)

    def find_with_root_and_path(self, root, path):
        return self.sceneGraph.FindWithRootAndPath(root, path)

    def get_node_content(self, node):
        return self.sceneGraph.GetNodeContent(node)

# Contains a dictionary to contain and export AZ.SceneAPI.Containers.SceneManifest
class SceneManifest():
    def __init__(self):
        self.manifest = {'values': []}

    def add_mesh_group(self, name) -> dict:
        meshGroup =  {}
        meshGroup['$type'] = '{07B356B7-3635-40B5-878A-FAC4EFD5AD86} MeshGroup'
        meshGroup['name'] = name
        meshGroup['nodeSelectionList'] = {'selectedNodes': [], 'unselectedNodes': []}
        meshGroup['rules'] = {'rules': [{'$type': 'MaterialRule'}]}
        self.manifest['values'].append(meshGroup)
        return meshGroup

    def add_prefab_group(self, name, id, json) -> dict:
        prefabGroup = {}
        prefabGroup['$type'] = '{99FE3C6F-5B55-4D8B-8013-2708010EC715} PrefabGroup'
        prefabGroup['name'] = name
        prefabGroup['id'] = id
        prefabGroup['prefabDomData'] = json
        self.manifest['values'].append(prefabGroup)
        return prefabGroup

    def mesh_group_select_node(self, meshGroup, nodeName):
        meshGroup['nodeSelectionList']['selectedNodes'].append(nodeName)

    def mesh_group_unselect_node(self, meshGroup, nodeName):
        meshGroup['nodeSelectionList']['unselectedNodes'].append(nodeName)

    def mesh_group_add_advanced_coordinate_system(self, meshGroup, originNodeName, translation, rotation, scale):
        originRule =  {}
        originRule['$type'] = 'CoordinateSystemRule'
        originRule['useAdvancedData'] = True
        originRule['originNodeName'] = '' if originNodeName is None else originNodeName
        if translation is not None:
            originRule['translation'] = translation
        if rotation is not None:
            originRule['rotation'] = rotation
        if scale != 1.0:
            originRule['scale'] = scale
        meshGroup['rules']['rules'].append(originRule)

    def mesh_group_add_comment(self, meshGroup, comment):
        commentRule =  {}
        commentRule['$type'] = 'CommentRule'
        commentRule['comment'] = comment
        meshGroup['rules']['rules'].append(commentRule)

    def export(self):
        return json.dumps(self.manifest)
