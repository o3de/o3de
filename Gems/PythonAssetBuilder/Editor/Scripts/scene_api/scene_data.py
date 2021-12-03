"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
import typing
import json
import azlmbr.scene as sceneApi
from enum import Enum, IntEnum


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


class PrimitiveShape(IntEnum):
    BEST_FIT = 0
    SPHERE = 1
    BOX = 2
    CAPSULE = 3


class DecompositionMode(IntEnum):
    VOXEL = 0
    TETRAHEDRON = 1


# Contains a dictionary to contain and export AZ.SceneAPI.Containers.SceneManifest
class SceneManifest():
    def __init__(self):
        self.manifest = {'values': []}

    def add_mesh_group(self, name: str) -> dict:
        meshGroup = {}
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

    def __default_or_value(self, val, default):
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
            'inverseMassesStreamName': self.__default_or_value(inverse_masses_stream_name, 'Default: 1.0')
        }

        if inverse_masses_channel is not None:
            cloth_rule['inverseMassesChannel'] = inverse_masses_channel
        cloth_rule['motionConstraintsStreamName'] = self.__default_or_value(motion_constraints_stream_name, 'Default: 1.0')
        if motion_constraints_channel is not None:
            cloth_rule['motionConstraintsChannel'] = motion_constraints_channel
        cloth_rule['backstopStreamName'] = self.__default_or_value(backstop_stream_name, 'None')
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
            'use32bitVertices': self.__default_or_value(use_32bit_vertices, False),
            'mergeMeshes': self.__default_or_value(merge_meshes, True),
            'useCustomNormals': self.__default_or_value(use_custom_normals, True)
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
            'maxWeightsPerVertex': self.__default_or_value(max_weights_per_vertex, 4),
            'weightThreshold': self.__default_or_value(weight_threshold, 0.001)
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
            'tangentSpace': self.__default_or_value(tangent_space, 1),
            'tSpaceMethod': self.__default_or_value(tspace_method, 0)
        }

        mesh_group['rules']['rules'].append(rule)

    def __add_physx_base_mesh_group(self, name: str, physics_material: typing.Optional[str]) -> dict:
        import azlmbr.math
        group = {
            '$type': '{5B03C8E6-8CEE-4DA0-A7FA-CD88689DD45B} MeshGroup',
            'id': azlmbr.math.Uuid_CreateRandom().ToString(),
            'name': name,
            'NodeSelectionList': {
                'selectedNodes': [],
                'unselectedNodes': []
            },
            "MaterialSlots": [
                "Material"
            ],
            "PhysicsMaterials": [
                self.__default_or_value(physics_material, "<Default Physics Material>")
            ],
            "rules": {
                "rules": []
            }
        }
        self.manifest['values'].append(group)

        return group

    def add_physx_triangle_mesh_group(self, name: str, merge_meshes: bool = True, weld_vertices: bool = False,
                                      disable_clean_mesh: bool = False,
                                      force_32bit_indices: bool = False,
                                      suppress_triangle_mesh_remap_table: bool = False,
                                      build_triangle_adjacencies: bool = False,
                                      mesh_weld_tolerance: float = 0.0,
                                      num_tris_per_leaf: int = 4,
                                      physics_material: typing.Optional[str] = None) -> dict:
        """
        Adds a Triangle type PhysX Mesh Group to the scene.

        :param name: Name of the mesh group.
        :param merge_meshes: When true, all selected nodes will be merged into a single collision mesh.
        :param weld_vertices: When true, mesh welding is performed. Clean mesh must be enabled.
        :param disable_clean_mesh: When true, mesh cleaning is disabled. This makes cooking faster.
        :param force_32bit_indices: When true, 32-bit indices will always be created regardless of triangle count.
        :param suppress_triangle_mesh_remap_table: When true, the face remap table is not created.
                            This saves a significant amount of memory, but the SDK will not be able to provide the remap
                            information for internal mesh triangles returned by collisions, sweeps or raycasts hits.
        :param build_triangle_adjacencies: When true, the triangle adjacency information is created.
        :param mesh_weld_tolerance: If mesh welding is enabled, this controls the distance at
                            which vertices are welded. If mesh welding is not enabled, this value defines the
                            acceptance distance for mesh validation. Provided no two vertices are within this
                            distance, the mesh is considered to be clean. If not, a warning will be emitted.
        :param num_tris_per_leaf: Mesh cooking hint for max triangles per leaf limit. Fewer triangles per leaf
                            produces larger meshes with better runtime performance and worse cooking performance.
        :param physics_material: Configure which physics material to use.
        :return: The newly created mesh group.
        """
        group = self.__add_physx_base_mesh_group(name, physics_material)
        group["export method"] = 0
        group["TriangleMeshAssetParams"] = {
            "MergeMeshes": merge_meshes,
            "WeldVertices": weld_vertices,
            "DisableCleanMesh": disable_clean_mesh,
            "Force32BitIndices": force_32bit_indices,
            "SuppressTriangleMeshRemapTable": suppress_triangle_mesh_remap_table,
            "BuildTriangleAdjacencies": build_triangle_adjacencies,
            "MeshWeldTolerance": mesh_weld_tolerance,
            "NumTrisPerLeaf": num_tris_per_leaf
        }

        return group

    def add_physx_convex_mesh_group(self, name: str, area_test_epsilon: float = 0.059, plane_tolerance: float = 0.0006,
                                    use_16bit_indices: bool = False,
                                    check_zero_area_triangles: bool = False,
                                    quantize_input: bool = False,
                                    use_plane_shifting: bool = False,
                                    shift_vertices: bool = False,
                                    gauss_map_limit: int = 32,
                                    build_gpu_data: bool = False,
                                    physics_material: typing.Optional[str] = None) -> dict:
        """
        Adds a Convex type PhysX Mesh Group to the scene.

        :param name: Name of the mesh group.
        :param area_test_epsilon: If the area of a triangle of the hull is below this value, the triangle will be
                            rejected. This test is done only if Check Zero Area Triangles is used.
        :param plane_tolerance: The value is used during hull construction. When a new point is about to be added
                            to the hull it gets dropped when the point is closer to the hull than the planeTolerance.
        :param use_16bit_indices: Denotes the use of 16-bit vertex indices in Convex triangles or polygons.
        :param check_zero_area_triangles: Checks and removes almost zero-area triangles during convex hull computation.
                            The rejected area size is specified in Area Test Epsilon.
        :param quantize_input: Quantizes the input vertices using the k-means clustering.
        :param use_plane_shifting: Enables plane shifting vertex limit algorithm. Plane shifting is an alternative
                            algorithm for the case when the computed hull has more vertices than the specified vertex
                            limit.
        :param shift_vertices: Convex hull input vertices are shifted to be around origin to provide better
                            computation stability
        :param gauss_map_limit: Vertex limit beyond which additional acceleration structures are computed for each
                            convex mesh. Increase that limit to reduce memory usage. Computing the extra structures
                            all the time does not guarantee optimal performance.
        :param build_gpu_data: When true, additional information required for GPU-accelerated rigid body
                            simulation is created. This can increase memory usage and cooking times for convex meshes
                            and triangle meshes. Convex hulls are created with respect to GPU simulation limitations.
                            Vertex limit is set to 64 and vertex limit per face is internally set to 32.
        :param physics_material: Configure which physics material to use.
        :return: The newly created mesh group.
        """
        group = self.__add_physx_base_mesh_group(name, physics_material)
        group["export method"] = 1
        group["ConvexAssetParams"] = {
            "AreaTestEpsilon": area_test_epsilon,
            "PlaneTolerance": plane_tolerance,
            "Use16bitIndices": use_16bit_indices,
            "CheckZeroAreaTriangles": check_zero_area_triangles,
            "QuantizeInput": quantize_input,
            "UsePlaneShifting": use_plane_shifting,
            "ShiftVertices": shift_vertices,
            "GaussMapLimit": gauss_map_limit,
            "BuildGpuData": build_gpu_data
        }

        return group

    def add_physx_primitive_mesh_group(self, name: str,
                                       primitive_shape_target: PrimitiveShape = PrimitiveShape.BEST_FIT,
                                       volume_term_coefficient: float = 0.0,
                                       physics_material: typing.Optional[str] = None) -> dict:
        """
        Adds a Primitive Shape type PhysX Mesh Group to the scene

        :param name: Name of the mesh group.
        :param primitive_shape_target: The shape that should be fitted to this mesh. If BEST_FIT is selected, the
                            algorithm will determine which of the shapes fits best.
        :param volume_term_coefficient: This parameter controls how aggressively the primitive fitting algorithm will try
                            to minimize the volume of the fitted primitive. A value of 0 (no volume minimization) is
                            recommended for most meshes, especially those with moderate to high vertex counts.
        :param physics_material: Configure which physics material to use.
        :return: The newly created mesh group.
        """
        group = self.__add_physx_base_mesh_group(name, physics_material)
        group["export method"] = 2
        group["PrimitiveAssetParams"] = {
            "PrimitiveShapeTarget": int(primitive_shape_target),
            "VolumeTermCoefficient": volume_term_coefficient
        }

        return group

    def physx_mesh_group_decompose_meshes(self, mesh_group: dict, max_convex_hulls: int = 1024,
                                          max_num_vertices_per_convex_hull: int = 64,
                                          concavity: float = .001,
                                          resolution: float = 100000,
                                          mode: DecompositionMode = DecompositionMode.VOXEL,
                                          alpha: float = .05,
                                          beta: float = .05,
                                          min_volume_per_convex_hull: float = 0.0001,
                                          plane_downsampling: int = 4,
                                          convex_hull_downsampling: int = 4,
                                          pca: bool = False,
                                          project_hull_vertices: bool = True) -> None:
        """
        Enables and configures mesh decomposition for a PhysX Mesh Group.
        Only valid for convex or primitive mesh types.

        :param mesh_group: Mesh group to configure decomposition for.
        :param max_convex_hulls: Controls the maximum number of hulls to generate.
        :param max_num_vertices_per_convex_hull: Controls the maximum number of triangles per convex hull.
        :param concavity: Maximum concavity of each approximate convex hull.
        :param resolution: Maximum number of voxels generated during the voxelization stage.
        :param mode: Select voxel-based approximate convex decomposition or tetrahedron-based
                            approximate convex decomposition.
        :param alpha: Controls the bias toward clipping along symmetry planes.
        :param beta: Controls the bias toward clipping along revolution axes.
        :param min_volume_per_convex_hull: Controls the adaptive sampling of the generated convex hulls.
        :param plane_downsampling: Controls the granularity of the search for the best clipping plane.
        :param convex_hull_downsampling: Controls the precision of the convex hull generation process
                            during the clipping plane selection stage.
        :param pca: Enable or disable normalizing the mesh before applying the convex decomposition.
        :param project_hull_vertices: Project the output convex hull vertices onto the original source mesh to increase
                            the floating point accuracy of the results.
        """
        mesh_group['DecomposeMeshes'] = True
        mesh_group['ConvexDecompositionParams'] = {
            "MaxConvexHulls": max_convex_hulls,
            "MaxNumVerticesPerConvexHull": max_num_vertices_per_convex_hull,
            "Concavity": concavity,
            "Resolution": resolution,
            "Mode": int(mode),
            "Alpha": alpha,
            "Beta": beta,
            "MinVolumePerConvexHull": min_volume_per_convex_hull,
            "PlaneDownsampling": plane_downsampling,
            "ConvexHullDownsampling": convex_hull_downsampling,
            "PCA": pca,
            "ProjectHullVertices": project_hull_vertices
        }

    def physx_mesh_group_add_selected_node(self, mesh_group: dict, node: str) -> None:
        """
        Adds a node to the selected nodes list

        :param mesh_group: Mesh group to add to.
        :param node: Node path to add.
        """
        mesh_group['NodeSelectionList']['selectedNodes'].append(node)

    def physx_mesh_group_add_unselected_node(self, mesh_group: dict, node: str) -> None:
        """
        Adds a node to the unselected nodes list

        :param mesh_group: Mesh group to add to.
        :param node: Node path to add.
        """
        mesh_group['NodeSelectionList']['unselectedNodes'].append(node)

    def physx_mesh_group_add_selected_unselected_nodes(self, mesh_group: dict, selected: typing.List[str],
                                                       unselected: typing.List[str]) -> None:
        """
        Adds a set of nodes to the selected/unselected node lists

        :param mesh_group: Mesh group to add to.
        :param selected: List of node paths to add to the selected list.
        :param unselected: List of node paths to add to the unselected list.
        """
        mesh_group['NodeSelectionList']['selectedNodes'].extend(selected)
        mesh_group['NodeSelectionList']['unselectedNodes'].extend(unselected)

    def physx_mesh_group_add_comment(self, mesh_group: dict, comment: str) -> None:
        """
        Adds a comment rule

        :param mesh_group: Mesh group to add the rule to.
        :param comment: Comment string.
        """
        rule = {
            "$type": "CommentRule",
            "comment": comment
        }
        mesh_group['rules']['rules'].append(rule)

    def export(self):
        return json.dumps(self.manifest)
