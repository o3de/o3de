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

    def DefaultOrValue(self, val, default):
        return default if val is None else val

    # 0 Red
    # 1 Green
    # 2 Blue
    # 3 Alpha
    def mesh_group_add_cloth_rule(self, meshGroup, clothNodeName,
        inverseMassesStreamName, inverseMassesChannel,
        motionConstrainsStreamName, motionConstraintsChannel, 
        backstopStreamName, backstopOffsetChannel, backstopRadiusChannel):
        cloth_rule = {}
        cloth_rule['$type'] = 'ClothRule'
        cloth_rule['meshNodeName'] = clothNodeName
        cloth_rule['inverseMassesStreamName'] = self.DefaultOrValue(inverseMassesStreamName, 'Default: 1.0')
        if inverseMassesChannel is not None:
            cloth_rule['inverseMassesChannel'] = inverseMassesChannel
        cloth_rule['motionConstraintsStreamName'] = self.DefaultOrValue(motionConstrainsStreamName, 'Default: 1.0')
        if motionConstraintsChannel is not None:
            cloth_rule['motionConstraintsChannel'] = motionConstraintsChannel
        cloth_rule['backstopStreamName'] = self.DefaultOrValue(backstopStreamName, 'None')
        if backstopOffsetChannel is not None:
            cloth_rule['backstopOffsetChannel'] = backstopOffsetChannel
        if backstopRadiusChannel is not None:
            cloth_rule['backstopRadiusChannel'] = backstopRadiusChannel
        meshGroup['rules']['rules'].append(cloth_rule)

    def mesh_group_add_lod_rule(self, meshGroup):
        lod_rule = {}
        lod_rule['$type'] = '{6E796AC8-1484-4909-860A-6D3F22A7346F} LodRule'
        lod_rule['nodeSelectionList'] = []
        meshGroup['rules']['rules'].append(lod_rule)
        return lod_rule

    def lod_rule_add_lod(self, lodRule):
        lod = {'selectedNodes': [], 'unselectedNodes': []}
        lodRule['nodeSelectionList'].append(lod)
        return lod

    def lod_select_node(self, lod, selectedNode):
        lod['selectedNodes'].append(selectedNode)

    def lod_unselect_node(self, lod, unselectedNode):
        lod['unselectedNodes'].append(unselectedNode)

    def mesh_group_add_advanced_mesh_rule(self, mesh_group, use_32bit_vertices, merge_meshes, use_custom_normals, vertex_color_stream):
        rule = {}
        rule['$type'] = 'StaticMeshAdvancedRule'
        rule['use32bitVertices'] = self.DefaultOrValue(use_32bit_vertices, False)
        rule['mergeMeshes'] = self.DefaultOrValue(merge_meshes, True)
        rule['useCustomNormals'] = self.DefaultOrValue(use_custom_normals, True)

        if vertex_color_stream is not None:
            rule['vertexColorStreamName'] = vertex_color_stream

        mesh_group['rules']['rules'].append(rule)

    def mesh_group_add_skin_rule(self, mesh_group, max_weights_per_vertex, weight_threshold):
        rule = {}
        rule['$type'] = 'SkinRule'
        rule['maxWeightsPerVertex'] = self.DefaultOrValue(max_weights_per_vertex, 4)
        rule['weightThreshold'] = self.DefaultOrValue(weight_threshold, 0.001)

        mesh_group['rules']['rules'].append(rule)

    def mesh_group_add_tangent_rule(self, mesh_group, tangent_space, tspace_method):
        rule = {}
        rule['$type'] = 'TangentsRule'
        rule['tangentSpace'] = self.DefaultOrValue(tangent_space, 1)
        rule['tSpaceMethod'] = self.DefaultOrValue(tspace_method, 0)
        mesh_group['rules']['rules'].append(rule)

    def export(self):
        return json.dumps(self.manifest)
