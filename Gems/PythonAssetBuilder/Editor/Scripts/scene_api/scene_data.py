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

    def add_mesh_group(self, name: str) -> dict:
        meshGroup =  {}
        meshGroup['$type'] = '{07B356B7-3635-40B5-878A-FAC4EFD5AD86} MeshGroup'
        meshGroup['name'] = name
        meshGroup['nodeSelectionList'] = {'selectedNodes': [], 'unselectedNodes': []}
        meshGroup['rules'] = {'rules': [{'$type': 'MaterialRule'}]}
        self.manifest['values'].append(meshGroup)
        return meshGroup

    def add_prefab_group(self, name: str, id: str, json: dict) -> dict:
        prefabGroup = {}
        prefabGroup['$type'] = '{99FE3C6F-5B55-4D8B-8013-2708010EC715} PrefabGroup'
        prefabGroup['name'] = name
        prefabGroup['id'] = id
        prefabGroup['prefabDomData'] = json
        self.manifest['values'].append(prefabGroup)
        return prefabGroup

    def mesh_group_select_node(self, mesh_group: dict, node_name: str) -> None:
        mesh_group['nodeSelectionList']['selectedNodes'].append(node_name)

    def mesh_group_unselect_node(self, mesh_group: dict, node_name: str) -> None:
        mesh_group['nodeSelectionList']['unselectedNodes'].append(node_name)

    def mesh_group_add_advanced_coordinate_system(self, mesh_group: dict, origin_node_name: str, translation: object,
                                                  rotation: object, scale: float) -> None:
        origin_rule = {
            '$type': 'CoordinateSystemRule',
            'useAdvancedData': True,
            'originNodeName': '' if origin_node_name is None else origin_node_name
        }
        if translation is not None:
            origin_rule['translation'] = translation
        if rotation is not None:
            origin_rule['rotation'] = rotation
        if scale != 1.0:
            origin_rule['scale'] = scale
        mesh_group['rules']['rules'].append(origin_rule)

    def mesh_group_add_comment(self, mesh_group: dict, comment: str) -> None:
        commentRule = {
            '$type': 'CommentRule',
            'comment': comment
        }
        mesh_group['rules']['rules'].append(commentRule)

    def DefaultOrValue(self, val, default):
        return default if val is None else val

    def mesh_group_add_cloth_rule(self, mesh_group: dict, cloth_node_name: str,
                                  inverse_masses_stream_name: str, inverse_masses_channel: int,
                                  motion_constraints_stream_name: str, motion_constraints_channel: int,
                                  backstop_stream_name: str, backstop_offset_channel: int,
                                  backstop_radius_channel: int) -> None:
        """
        Adds a Cloth rule. 0 = Red, 1 = Green, 2 = Blue, 3 = Alpha
        :param mesh_group: Mesh Group to add the cloth rule to
        :param cloth_node_name: Name of the node that the rule applies to
        :param inverse_masses_stream_name: Name of the color stream to use for inverse masses
        :param inverse_masses_channel: Color channel (index) for inverse masses
        :param motion_constraints_stream_name: Name of the color stream to use for motion constraints
        :param motion_constraints_channel: Color channel (index) for motion constraints
        :param backstop_stream_name: Name of the color stream to use for backstop
        :param backstop_offset_channel: Color channel (index) for backstop offset value
        :param backstop_radius_channel: Color chnanel (index) for backstop radius value
        """
        cloth_rule = {
            '$type': 'ClothRule',
            'meshNodeName': cloth_node_name,
            'inverseMassesStreamName': self.DefaultOrValue(inverse_masses_stream_name, 'Default: 1.0')
        }

        if inverse_masses_channel is not None:
            cloth_rule['inverseMassesChannel'] = inverse_masses_channel
        cloth_rule['motionConstraintsStreamName'] = self.DefaultOrValue(motion_constraints_stream_name, 'Default: 1.0')
        if motion_constraints_channel is not None:
            cloth_rule['motionConstraintsChannel'] = motion_constraints_channel
        cloth_rule['backstopStreamName'] = self.DefaultOrValue(backstop_stream_name, 'None')
        if backstop_offset_channel is not None:
            cloth_rule['backstopOffsetChannel'] = backstop_offset_channel
        if backstop_radius_channel is not None:
            cloth_rule['backstopRadiusChannel'] = backstop_radius_channel
        mesh_group['rules']['rules'].append(cloth_rule)

    def mesh_group_add_lod_rule(self, mesh_group: dict) -> dict:
        """
        Adds an LOD rule
        :param mesh_group: Mesh Group to add the rule to
        :return: LOD rule
        """
        lod_rule = {
            '$type': '{6E796AC8-1484-4909-860A-6D3F22A7346F} LodRule',
            'nodeSelectionList': []
        }

        mesh_group['rules']['rules'].append(lod_rule)
        return lod_rule

    def lod_rule_add_lod(self, lod_rule: dict) -> dict:
        """
        Adds an LOD level to the LOD rule.  Nodes are added in order.  The first node added represents LOD1, 2nd LOD2, etc
        :param lod_rule: LOD rule to add the LOD level to
        :return: LOD level
        """
        lod = {'selectedNodes': [], 'unselectedNodes': []}
        lod_rule['nodeSelectionList'].append(lod)
        return lod

    def lod_select_node(self, lod: dict, selected_node: str) -> None:
        """
        Adds a node as a selected node
        :param lod: LOD level to add the node to
        :param selected_node: Path of the node
        """
        lod['selectedNodes'].append(selected_node)

    def lod_unselect_node(self, lod: dict, unselected_node: str) -> None:
        """
        Adds a node as an unselected node
        :param lod: LOD rule to add the node to
        :param unselected_node: Path of the node
        """
        lod['unselectedNodes'].append(unselected_node)

    def mesh_group_add_advanced_mesh_rule(self, mesh_group: dict, use_32bit_vertices: bool, merge_meshes: bool,
                                          use_custom_normals: bool,
                                          vertex_color_stream: str) -> None:
        """
        Adds an Advanced Mesh rule
        :param mesh_group: Mesh Group to add the rule to
        :param use_32bit_vertices: False = 16bit vertex position precision. True = 32bit vertex position precision
        :param merge_meshes: Merge all meshes into a single mesh
        :param use_custom_normals: True = use normals from DCC tool.  False = average normals
        :param vertex_color_stream: Color stream name to use for Vertex Coloring
        """
        rule = {
            '$type': 'StaticMeshAdvancedRule',
            'use32bitVertices': self.DefaultOrValue(use_32bit_vertices, False),
            'mergeMeshes': self.DefaultOrValue(merge_meshes, True),
            'useCustomNormals': self.DefaultOrValue(use_custom_normals, True)
        }

        if vertex_color_stream is not None:
            rule['vertexColorStreamName'] = vertex_color_stream

        mesh_group['rules']['rules'].append(rule)

    def mesh_group_add_skin_rule(self, mesh_group: dict, max_weights_per_vertex: int, weight_threshold: float) -> None:
        """
        Adds a Skin rule
        :param mesh_group: Mesh Group to add the rule to
        :param max_weights_per_vertex: Max number of joints that can influence a vertex
        :param weight_threshold: Weight values below this value will be treated as 0
        """
        rule = {
            '$type': 'SkinRule',
            'maxWeightsPerVertex': self.DefaultOrValue(max_weights_per_vertex, 4),
            'weightThreshold': self.DefaultOrValue(weight_threshold, 0.001)
        }

        mesh_group['rules']['rules'].append(rule)

    def mesh_group_add_tangent_rule(self, mesh_group: dict, tangent_space: int, tspace_method: int) -> None:
        """
        Adds a Tangent rule to control tangent space generation
        :param mesh_group: Mesh Group to add the rule to
        :param tangent_space: Tangent space source. 0 = Scene, 1 = MikkT Tangent Generation
        :param tspace_method: MikkT Generation method. 0 = TSpace, 1 = TSpaceBasic
        """
        rule = {
            '$type': 'TangentsRule',
            'tangentSpace': self.DefaultOrValue(tangent_space, 1),
            'tSpaceMethod': self.DefaultOrValue(tspace_method, 0)
        }

        mesh_group['rules']['rules'].append(rule)

    def export(self):
        return json.dumps(self.manifest)
