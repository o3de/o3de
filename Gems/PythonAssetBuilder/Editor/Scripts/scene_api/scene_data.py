"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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

    def mesh_group_select_node(self, meshGroup, nodeName):
        meshGroup['nodeSelectionList']['selectedNodes'].append(nodeName)

    def mesh_group_unselect_node(self, meshGroup, nodeName):
        meshGroup['nodeSelectionList']['unselectedNodes'].append(nodeName)

    def mesh_group_set_origin(self, meshGroup, originNodeName, x, y, z, scale):
        originRule =  {}
        originRule['$type'] = 'OriginRule'
        originRule['originNodeName'] = 'World' if originNodeName is None else originNodeName
        originRule['translation'] = [x, y, z]
        originRule['scale'] = scale
        meshGroup['rules']['rules'].append(originRule)

    def mesh_group_add_comment(self, meshGroup, comment):
        commentRule =  {}
        commentRule['$type'] = 'CommentRule'
        commentRule['comment'] = comment
        meshGroup['rules']['rules'].append(commentRule)

    def export(self):
        return json.dumps(self.manifest)
