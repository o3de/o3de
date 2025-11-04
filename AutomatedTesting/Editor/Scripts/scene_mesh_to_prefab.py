#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
import azlmbr.bus
import azlmbr.math

from scene_api.scene_data import PrimitiveShape, DecompositionMode, ColorChannel, TangentSpaceSource, TangentSpaceMethod
from scene_helpers import *


#
# SceneAPI Processor
#

def add_material_component(entity_id):
    # Create an override AZ::Render::EditorMaterialComponent
    editor_material_component = azlmbr.entity.EntityUtilityBus(
        azlmbr.bus.Broadcast,
        "GetOrAddComponentByTypeName",
        entity_id,
        "EditorMaterialComponent")

    # this fills out the material asset to a known product AZMaterial asset relative path
    json_update = json.dumps({
        "Controller": {"Configuration": {"materials": [
            {
                "Key": {},
                "Value": {"MaterialAsset": {
                    "assetHint": "materials/basic_grey.azmaterial"
                }}
            }]
        }}
    })
    result = azlmbr.entity.EntityUtilityBus(azlmbr.bus.Broadcast, "UpdateComponentForEntity", entity_id,
                                            editor_material_component, json_update)

    if not result:
        raise RuntimeError("UpdateComponentForEntity for editor_material_component failed")


def add_physx_meshes(scene_manifest: sceneData.SceneManifest, source_file_name: str, mesh_name_list: List, all_node_paths: List[str]):
    import uuid

    first_mesh = mesh_name_list[0].get_path()

    # Add a Box Primitive PhysX mesh with a comment
    physx_box_group = scene_manifest.add_physx_primitive_mesh_group(source_file_name + "_box", PrimitiveShape.BOX, 0.0, None)
    # Give a unique ID the PhysX mesh group
    physx_box_group['id'] = '{' + str(uuid.uuid5(uuid.NAMESPACE_DNS, source_file_name + "_box")) + '}'
    scene_manifest.physx_mesh_group_add_comment(physx_box_group, "This is a box primitive")
    # Select the first mesh, unselect every other node
    scene_manifest.physx_mesh_group_add_selected_node(physx_box_group, first_mesh)

    for node in all_node_paths:
        if node != first_mesh:
            scene_manifest.physx_mesh_group_add_unselected_node(physx_box_group, node)

    # Add a Convex Mesh PhysX mesh with a comment
    physx_convex_mesh_group = scene_manifest.add_physx_convex_mesh_group(source_file_name + "_convex",
                                                             area_test_epsilon = 0.08,
                                                             plane_tolerance = .0004,
                                                             use_16bit_indices = True,
                                                             check_zero_area_triangles = True,
                                                             quantize_input = True,
                                                             use_plane_shifting = True,
                                                             shift_vertices = True,
                                                             gauss_map_limit = 24,
                                                             build_gpu_data = True,
                                                             physics_material_asset_hint = "physx/glass.physicsmaterial")
    # Give a unique ID the PhysX mesh group
    physx_convex_mesh_group['id'] = '{' + str(uuid.uuid5(uuid.NAMESPACE_DNS, source_file_name + "_convex")) + '}'
    scene_manifest.physx_mesh_group_add_comment(physx_convex_mesh_group, "This is a convex mesh")
    # Select/Unselect nodes using lists
    all_except_first_mesh = [x for x in all_node_paths if x != first_mesh]
    scene_manifest.physx_mesh_group_add_selected_unselected_nodes(physx_convex_mesh_group, [first_mesh], all_except_first_mesh)

    # Configure mesh decomposition for this mesh
    scene_manifest.physx_mesh_group_decompose_meshes(physx_convex_mesh_group, 512, 32, .002, 100100, DecompositionMode.TETRAHEDRON,
                                                     0.06, 0.055, 0.00015, 3, 3, True, False)

    # Add a Triangle mesh
    physx_triangle_mesh_group = scene_manifest.add_physx_triangle_mesh_group(source_file_name + "_triangle", 
                                                                             merge_meshes = False,
                                                                             weld_vertices = True,
                                                                             disable_clean_mesh = True,
                                                                             force_32bit_indices = True,
                                                                             suppress_triangle_mesh_remap_table = True,
                                                                             build_triangle_adjacencies = True)
    # Give a unique ID the PhysX mesh group
    physx_triangle_mesh_group['id'] = '{' + str(uuid.uuid5(uuid.NAMESPACE_DNS, source_file_name + "_triangle")) + '}'
    scene_manifest.physx_mesh_group_add_selected_unselected_nodes(physx_triangle_mesh_group, [first_mesh], all_except_first_mesh)


def update_manifest(scene):
    import uuid, os
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

    add_physx_meshes(scene_manifest, source_filename_only, mesh_name_list, all_node_paths)

    # Loop every mesh node in the scene
    for activeMeshIndex in range(len(mesh_name_list)):
        mesh_name = mesh_name_list[activeMeshIndex]
        mesh_path = mesh_name.get_path()
        # Create a unique mesh group name using the filename + node name
        mesh_group_name = '{}_{}'.format(source_filename_only, mesh_name.get_name())
        # Remove forbidden filename characters from the name since this will become a file on disk later
        mesh_group_name = sanitize_name_for_disk(mesh_group_name)
        # Add the MeshGroup to the manifest and give it a unique ID
        mesh_group = scene_manifest.add_mesh_group(mesh_group_name)
        mesh_group['id'] = '{' + str(uuid.uuid5(uuid.NAMESPACE_DNS, source_filename_only + mesh_path)) + '}'
        # Set our current node as the only node that is included in this MeshGroup
        scene_manifest.mesh_group_select_node(mesh_group, mesh_path)
        scene_manifest.mesh_group_add_comment(mesh_group, "Hello World")

        # Explicitly remove all other nodes to prevent implicit inclusions
        for node in all_node_paths:
            if node != mesh_path:
                scene_manifest.mesh_group_unselect_node(mesh_group, node)

        scene_manifest.mesh_group_add_cloth_rule(mesh_group, mesh_path, "Col0", ColorChannel.GREEN, "Col0",
                                                 ColorChannel.BLUE, "Col0", ColorChannel.BLUE, ColorChannel.ALPHA)
        scene_manifest.mesh_group_add_advanced_mesh_rule(mesh_group, True, False, True, "Col0")
        scene_manifest.mesh_group_add_skin_rule(mesh_group, 3, 0.002)
        scene_manifest.mesh_group_add_tangent_rule(mesh_group, TangentSpaceSource.MIKKT_GENERATION, TangentSpaceMethod.TSPACE_BASIC)

        # Create an editor entity
        entity_id = azlmbr.entity.EntityUtilityBus(azlmbr.bus.Broadcast, "CreateEditorReadyEntity", mesh_group_name)
        # Add an EditorMeshComponent to the entity
        editor_mesh_component = azlmbr.entity.EntityUtilityBus(azlmbr.bus.Broadcast, "GetOrAddComponentByTypeName",
                                                               entity_id, "AZ::Render::EditorMeshComponent")
        # Set the ModelAsset assetHint to the relative path of the input asset + the name of the MeshGroup we just
        # created + the azmodel extension The MeshGroup we created will be output as a product in the asset's path
        # named mesh_group_name.fbx.azmodel The assetHint will be converted to an AssetId later during prefab loading
        json_update = json.dumps({
            "Controller": {"Configuration": {"ModelAsset": {
                "assetHint": os.path.join(source_relative_path, mesh_group_name) + ".fbx.azmodel"}}}
        })
        # Apply the JSON above to the component we created
        result = azlmbr.entity.EntityUtilityBus(azlmbr.bus.Broadcast, "UpdateComponentForEntity", entity_id,
                                                editor_mesh_component, json_update)

        if not result:
            raise RuntimeError("UpdateComponentForEntity failed for Mesh component")

        # Add a physics component referencing the triangle mesh we made for the first node
        if previous_entity_id is None:
            physx_mesh_component = azlmbr.entity.EntityUtilityBus(azlmbr.bus.Broadcast, "GetOrAddComponentByTypeName",
                                      entity_id, "{20382794-0E74-4860-9C35-A19F22DC80D4} EditorMeshColliderComponent")

            json_update = json.dumps({
                "ShapeConfiguration": {
                    "PhysicsAsset": {
                        "Asset": {
                            "assetHint": os.path.join(source_relative_path, source_filename_only + "_triangle.fbx.pxmesh")
                        }
                    }
                }
            })

            result = azlmbr.entity.EntityUtilityBus(azlmbr.bus.Broadcast, "UpdateComponentForEntity", entity_id, physx_mesh_component, json_update)

            if not result:
                raise RuntimeError("UpdateComponentForEntity failed for PhysX mesh component")

        # an example of adding a material component to override the default material
        if previous_entity_id is not None and first_mesh:
            first_mesh = False
            add_material_component(entity_id)

        # Get the transform component
        transform_component = azlmbr.entity.EntityUtilityBus(azlmbr.bus.Broadcast, "GetOrAddComponentByTypeName",
                                                             entity_id, "27F1E1A1-8D9D-4C3B-BD3A-AFB9762449C0")

        # Set this entity to be a child of the last entity we created
        # This is just an example of how to do parenting and isn't necessarily useful to parent everything like this
        if previous_entity_id is not None:
            transform_json = json.dumps({
                "Parent Entity": previous_entity_id.to_json()
            })

            # Apply the JSON update
            result = azlmbr.entity.EntityUtilityBus(azlmbr.bus.Broadcast, "UpdateComponentForEntity", entity_id,
                                                    transform_component, transform_json)

            if not result:
                raise RuntimeError("UpdateComponentForEntity failed for Transform component")

        # Update the last entity id for next time
        previous_entity_id = entity_id

        # Keep track of the entity we set up, we'll add them all to the prefab we're creating later
        created_entities.append(entity_id)

    # Create a prefab with all our entities
    create_prefab(scene_manifest, source_filename_only, created_entities)

    # Convert the manifest to a JSON string and return it
    new_manifest = scene_manifest.export()

    return new_manifest


sceneJobHandler = None


def on_update_manifest(args):
    try:
        scene = args[0]
        data = update_manifest(scene)
    except RuntimeError as err:
        print(f'ERROR - {err}')
        log_exception_traceback()
    except:
        log_exception_traceback()

    global sceneJobHandler
    # do not delete or set sceneJobHandler to None, just disconnect from it.
    # this call is occuring while the scene Job Handler itself is in the callstack, so deleting it here
    # would cause a crash.
    sceneJobHandler.disconnect()
    return data


# try to create SceneAPI handler for processing
try:
    import azlmbr.scene as sceneApi
    
    sceneJobHandler = sceneApi.ScriptBuildingNotificationBusHandler()
    sceneJobHandler.connect()
    sceneJobHandler.add_callback('OnUpdateManifest', on_update_manifest)
except:
    sceneJobHandler = None
