# coding:utf-8
#!/usr/bin/python
#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
# -------------------------------------------------------------------------
import bpy
from bpy.props import EnumProperty
import shutil
from pathlib import Path
import re

from . import constants
from . import ui
from . import o3de_utils

def select_object(obj):
    """!
    This function will select just one object
    """
    bpy.ops.object.select_all(action='DESELECT')
    bpy.context.view_layer.objects.active = obj
    obj.select_set(True)

def check_selected():
    """!
    This function will check to see if the user has selected a object
    """
    selections = [obj.name for obj in bpy.context.selected_objects]
    if selections == []:
        return False, "None"
    else:
        return True, selections

def check_selected_bone_names():
    """!
    This function will check to see if there is a ARMATURE selected and if the bone names are o3de compatable.
    """
    context = bpy.context
    obj = context.object

    invalid_bone_names = None
    bone_are_in_selected = None

    for selected_obj in context.selected_objects:
        if selected_obj is not []:
            if selected_obj.type == "ARMATURE":
                bone_are_in_selected = True
                try:
                    for pb in obj.pose.bones:
                        # Validate all chars in string.
                        check_re =  re.compile(r"^[^<>/{}[\]~.`]*$")
                        if not check_re.match(pb.name):
                            # If any in the ARMATURE are named invalid return will fail.
                            invalid_bone_names = False
                except AttributeError:
                    pass
    return invalid_bone_names, bone_are_in_selected

def selected_hierarchy_and_rig_animation():
    """!
    This function will select the attached animation armature rig and all of its children.
    """
    obj = bpy.ops.object
    
    for selected_obj in bpy.context.selected_objects:
        if selected_obj is not []:
            obj.select_grouped(extend=True, type='SIBLINGS')
            obj.select_grouped(extend=True, type='PARENT')
            obj.select_grouped(extend=True, type='CHILDREN_RECURSIVE')
            obj.select_grouped(extend=True, type='PARENT')

def selected_attachment_and_rig_animation():
    """!
    This function will select the selected attachments armature rig.
    """
    obj = bpy.ops.object
    
    for selected_obj in bpy.context.selected_objects:
        if selected_obj is not []:
            obj.select_grouped(extend=True, type='PARENT')

def valid_animation_selection():
    """!
    This function will check to see if user has selected a valid level in
    the hierarchy for exporting the selected mesh, armature rig and animation.
    """
    object_selections = [obj for obj in bpy.context.selected_objects]
    if object_selections:
        obj = object_selections[0]

        if obj.type == 'ARMATURE':
            bpy.types.Scene.export_good_o3de = False
            ui.message_box("You must select lowest level mesh of Armature.", "O3DE Tools", "ERROR")
        elif obj.children:
            bpy.types.Scene.export_good_o3de = False
            ui.message_box("You need to select base mesh level.", "O3DE Tools", "ERROR")
        else:
            bpy.types.Scene.export_good_o3de = True
            if bpy.types.Scene.animation_export == constants.SKIN_ATTACHMENT:
                selected_attachment_and_rig_animation()
            else:
                selected_hierarchy_and_rig_animation()

def check_for_animation_actions():
    """!
    This function will check to see if the current scene has animation actions
    return actions[0] is the top level animation action
    return len(actions) is the number of available actions
    """
    obj = bpy.context.object
    actions = []
    active_action_name = ""
    # Look for animation actions available
    for animations in bpy.data.actions:
        actions.append(animations.name)
    # Look for Active Animation Action
    try:
        if not obj.animation_data == None:
            active_action_name = obj.animation_data.action.name
        else:
            active_action_name = ""
    except AttributeError:
        pass
    return active_action_name, len(actions)

def check_if_valid_path(file_path):
    """!
    This function will check to see if a file path exist and return a bool
    """
    if Path(file_path).exists():
        return True
    else:
        return False

def get_selected_materials(selected):
    """!
    This function will check the selected mesh and find material are connected
    then build a material list for selected.
    @param selected This is your current selected mesh(s)
    """
    materials = []
    # Find Connected materials attached to mesh
    for obj in selected:
        try:
            materials = get_all_materials(obj, materials)
        except AttributeError:
            pass
    return materials

def get_all_materials(obj, materials):
    """!
    This function will loop through all assigned materails slots on a mesh and the attach textures.
    then build a material list for selected.
    @param selected This is your current selected mesh(s)
    """
    for mat in obj.material_slots:
        try:
            if mat.name not in materials:
                materials.append(mat.name)
        except AttributeError:
            pass
    return materials

def loop_through_selected_materials(texture_file_path):
    """!
    This function will loop the the selected materials and copy the files to the o3de project folder.
    @param texture_file_path This is the current o3de projects texture file path selected for texture export.
    """
    if not Path(texture_file_path).exists():
        Path(texture_file_path).mkdir(exist_ok=True)
    # retrive the list of seleted mesh materials
    selected_materials = get_selected_materials(bpy.context.selected_objects)
    # Loop through Materials
    for mat in selected_materials:
        if mat:
            # Get the material
            material = bpy.data.materials[mat]
            # Loop through material node tree and get all the texture iamges
            for img in material.node_tree.nodes:
                if img.type == 'TEX_IMAGE':
                    # First make sure the image is not packed inside blender
                    if img.image.packed_file:
                        if Path(img.image.name).suffix in constants.IMAGE_EXT:
                            bpy.data.images[img.image.name].filepath = str(Path(texture_file_path).joinpath(img.image.name))
                        else:
                            ext = '.png'
                            build_string = f'{img.image.name}{ext}'
                            bpy.data.images[img.image.name].filepath = str(Path(texture_file_path).joinpath(build_string))
                        
                        bpy.data.images[img.image.name].save()
                    full_path = Path(bpy.path.abspath(img.image.filepath, library=img.image.library))
                    base_name = Path(full_path).name
                    if not full_path.exists(): # if embedded path doesnt exist, check current folder
                        full_path = Path(bpy.data.filepath).parent.joinpath(base_name)
                    o3de_texture_path = Path(texture_file_path).joinpath(base_name)
                    # Add to stored_image_source_paths to replace later
                    if not check_file_paths_duplicate(full_path, o3de_texture_path): # We check first if its not already copied over.
                        bpy.types.Scene.stored_image_source_paths[img.image.name] = full_path
                        # Copy the image to Destination
                        try:
                            bpy.data.images[img.image.name].filepath = str(o3de_texture_path)
                            if full_path.exists():
                                copy_texture_file(full_path, o3de_texture_path)
                            else:
                                bpy.data.images[img.image.name].save() 
                                # Save image to location
                        except (FileNotFoundError, RuntimeError):
                            pass
                    img.image.reload()

def copy_texture_file(source_path, destination_path):
    """!
    This function will copy project texture files to a O3DE textures folder
    @param source_path This is the texture source path
    @param destination_path This is the O3DE destination path
    """
    destination_size = -1
    source_size = source_path.stat().st_size
    if destination_path.exists():
        destination_size = destination_path.stat().st_size
    if source_size != destination_size:
        shutil.copyfile(source_path, destination_path)

def clone_repath_images(fileMenuExport, texture_file_path, projectSelectionList):
    """!
    This function will copy project texture files to a
    O3DE textures folder and repath the Blender materials
    then repath them to thier orginal.
    @param fileMenuExport This is a bool to let us know if this is a Blender File Menu Export
    @param texture_file_path This is our o3de projects texture file export path
    @param projectSelectionList This is a list of our o3de projects
    """
    # Lets create a dictionary to store all the source paths to place back after export
    bpy.types.Scene.stored_image_source_paths = {}

    dir_path = Path(texture_file_path)
    # Make or do not make a texture folder
    if fileMenuExport:
        # FILE MENU MENU EXPORT
        if bpy.types.Scene.export_textures_folder:
            # We do want the texture folder
            texture_file_path = Path(dir_path.parent).joinpath('textures')
            if not Path(texture_file_path).exists():
                Path(texture_file_path).mkdir(exist_ok=True)
            loop_through_selected_materials(texture_file_path)
        else:
            # We do not want the texture folder
            texture_file_path = Path(dir_path.parent)
            loop_through_selected_materials(texture_file_path)
        # Check to see if this project is listed in our projectSelectionList if not add it.
        if bpy.types.Scene.selected_o3de_project_path not in projectSelectionList:
            o3de_utils.save_project_list_json(str(bpy.types.Scene.selected_o3de_project_path))
            o3de_utils.build_projects_list()
        bpy.types.Scene.o3de_projects_list = EnumProperty(items=projectSelectionList, name='')
    elif fileMenuExport is None:
        # TOOL MENU EXPORT BUT WAS EXPORTED ONCE IN FILE MENU
        if bpy.types.Scene.export_textures_folder:
            texture_file_path = Path(bpy.types.Scene.selected_o3de_project_path).joinpath('textures')
            loop_through_selected_materials(texture_file_path)
        else:
            texture_file_path = bpy.types.Scene.selected_o3de_project_path
            if not Path(texture_file_path).exists():
                Path(texture_file_path).mkdir(exist_ok=True)
            loop_through_selected_materials(texture_file_path)
    else:
        # TOOL MENU EXPORT
        if bpy.types.Scene.export_textures_folder:
            # We want the texture folder
            texture_file_path = Path(bpy.types.Scene.selected_o3de_project_path).joinpath('Assets', 'textures')
            loop_through_selected_materials(texture_file_path)
        else:
            # We do not the texture folder
            texture_file_path = Path(bpy.types.Scene.selected_o3de_project_path).joinpath('Assets')
            loop_through_selected_materials(texture_file_path)

def check_file_paths_duplicate(source_path, destination_path):
    """!
    This function check to see if Source Path and Dest Path are the same
    @param source_path This is the non o3de source texture path of the texture
    @param destination_path This is the destination o3de project texture path
    """
    try:
        check_file = Path(source_path)
        if check_file.samefile(destination_path):
            return True
        else:
            return False
    except FileNotFoundError:
        pass

def replace_stored_paths():
    """!
    This Function will replace all the repathed image paths to thier origninal source paths
    """
    for image in bpy.data.images:
        try:
            # Find image and replace source path
            bpy.data.images[image.name].filepath = str(bpy.types.Scene.stored_image_source_paths[image.name])
        except KeyError:
            pass
        image.reload()
    bpy.types.Scene.stored_image_source_paths = {}


def duplicate_selected(selected_objects, rename):
    """!
    This function will duplicate selected objects with a new name string
    @param selected_objects This is the current selected object
    @param rename this is the duplicate object name
    @param return duplicated object
    """
    # Duplicate the mesh and add add the UDP name extension
    duplicate_object = bpy.data.objects.new(f'{selected_objects.name}{bpy.types.Scene.udp_type}', bpy.data.objects[selected_objects.name].data)
    # Add copy to current collection in scene
    bpy.context.collection.objects.link(duplicate_object)
    # Rename duplicated object
    duplicate_object.name = rename
    duplicate_object.data.name = rename
    return duplicate_object

def check_for_blender_int_ext(duplicated_node_name):
    """!
    This function will check if blender adds on its own .000 ext to a node name
    @param duplicated_node_name This is the name string of the duplicate object
    @param return name string
    """
    if '.' in duplicated_node_name:
            name, ext = duplicated_node_name.split(".")
            node_name = name
    else:
        name = duplicated_node_name
    return name

def check_for_udp():
    """!
    This function will check if selected has and custom properties 
    Example: print( f"Value: {obj_upd_keys} Key: {bpy.data.objects[selected_obj.name][obj_upd_keys] } ")
    """
    context = bpy.context
    # Create list to store selected meshs udps
    lod_list = []
    colliders_list = []

    for selected_obj in context.selected_objects:
        if selected_obj is not []:
            if selected_obj.type == "MESH":
                for obj_upd_keys in bpy.data.objects[selected_obj.name].keys():
                    if obj_upd_keys not in '_RNA_UI':
                        if obj_upd_keys == "o3de_atom_lod":
                            lod_list.append(True)
                        if obj_upd_keys == "o3de_atom_phys":
                            colliders_list.append(True)
    lods = any(lod_list)
    colliders = any(colliders_list)      
    return lods, colliders

def create_udp():
    """!
    This function will create a copy of a selected mesh and create an o3de PhysX collider PhysX Mesh that will
    autodect in o3de if you have a PhysX Collder Component.
    """
    selected_objects = bpy.context.object
    # Check to see if bool was check on or of on pop-up question
    if bpy.types.Scene.pop_up_question_bool:
        # Check to see if name string already contails a _LOD int
        if bpy.types.Scene.udp_type == "_lod":
            if re.search(r"_lod(?:)\d", selected_objects.name):
                # Lets get the current LOD Level
                ext_result = re.search(r"_lod(?:)\d", selected_objects.name)
                lod_level_int = ext_result.group().split("_lod")
                # Lets get the level and add the next
                lod_level = int(lod_level_int[1]) + 1
                # rename to the correct lod level
                base_name = selected_objects.name.rsplit("_",1)
                # Duplicate the object
                duplicate_object = duplicate_selected(selected_objects, f"{base_name[0]}_lod{lod_level}")
                # ADD UDP
                duplicate_object["o3de_atom_lod"] = f"_lod{lod_level}"
            else:
                duplicate_object = duplicate_selected(selected_objects, f"{selected_objects.name}_lod0")
                # ADD UDP
                duplicate_object["o3de_atom_lod"] = "_lod0"
        # If UDP Type is _phys
        if bpy.types.Scene.udp_type == "_phys":
            duplicate_object = duplicate_selected(selected_objects, f"{selected_objects.name}_phys")
            duplicate_object["o3de_atom_phys"] = "_phys"
        # Select the duplicated object only
        select_object(duplicate_object)
        # Add the Decimate Modifier on for user. Since both _lod and _phys will need this modifier
        #  for now we will have it as a default.
        add_remove_modifier("DECIMATE", True)
    else:
        if bpy.types.Scene.udp_type == "_lod":
                selected_objects['o3de_atom_lod'] = "_lod0"
        if bpy.types.Scene.udp_type == "_phys":
            selected_objects["o3de_atom_phys"] = "_phys"

def compair_set(list_a, list_b):
    """!
    This function will check to see if there are any difference between two list
    """
    set_a = list_a
    set_b = list_b
    if set_a == set_b:
        return True
    else:
        return False

def add_remove_modifier(modifier_name, add):
    """!
    This function will add or remove a modifier to selected
    @param modifier_name is the name of the modifier you wish to add or remove
    @param add if Bool True will add the modifier, if False will remove 
    """
    context = bpy.context

    for selected_obj in context.selected_objects:
        if selected_obj is not []:
            if selected_obj.type == "MESH":
                if add:
                    # Set the mesh active
                    bpy.context.view_layer.objects.active = selected_obj
                    # Add Modifier
                    bpy.ops.object.modifier_add(type=modifier_name)
                    if modifier_name == "TRIANGULATE":
                        bpy.context.object.modifiers["Triangulate"].keep_custom_normals = True
                else:
                    # Set the mesh active
                    bpy.context.view_layer.objects.active = selected_obj
                    # Remove Modifier
                    bpy.ops.object.modifier_remove(modifier=modifier_name)

def check_selected_stats_count():
    """!
    This function will check selected and get poly counts
    """
    # We will make list for each selection
    mesh_vertices_list = []
    mesh_edges_list = []
    mesh_polygons_list = []
    mesh_material_list = []
    
    context = bpy.context
    
    for selected_obj in context.selected_objects:
        if selected_obj is not []:
            if selected_obj.type == "MESH":
                mesh_stats = selected_obj.data
                # Vert, Edge, Poly Counts
                mesh_vertices = len(mesh_stats.vertices)
                mesh_edges = len(mesh_stats.edges)
                mesh_polygons = len(mesh_stats.polygons)
                mesh_materials = len(mesh_stats.materials)
                mesh_vertices_list.append(mesh_vertices)
                mesh_edges_list.append(mesh_edges)
                mesh_polygons_list.append(mesh_polygons)
                # Material Count
                mesh_material_list.append(mesh_materials)

    vert_count = sum(mesh_vertices_list)
    edge_count = sum(mesh_edges_list)
    polygon_count = sum(mesh_polygons_list)
    material_count = sum(mesh_material_list)
    # Reset the list
    mesh_vertices_list = []
    mesh_edges_list = []
    mesh_polygons_list = []
    mesh_material_list = []
    return vert_count, edge_count, polygon_count, material_count

def check_selected_uvs():
    """!
    This function will check to see if there are UVs
    """
    context = bpy.context
    # We will make list for each selection and compair if there are UVs.
    mesh_uv_list = []
    
    for selected_obj in context.selected_objects:
        if selected_obj is not []:
            if selected_obj.type == "MESH":
                if selected_obj.data.uv_layers:
                    mesh_uv_list.append(True)
                else:
                    mesh_uv_list.append(False)
    object_uv_list = all(mesh_uv_list)
    # If all objects have UVs
    if object_uv_list:
        # Clear UV List
        mesh_uv_list = []
        return True
    else:
        # Clear UV List
        mesh_uv_list = []
        return False

def check_selected_transforms():
    """!
    This function will check to see if there are unfrozen transfors and to warn the artist before export.
    """
    context = bpy.context
    # We will make list for each selection and compair to a frozzen transfrom.
    location_list = []
    location_source = [0.0, 0.0, 0.0]
    rotation_list = []
    rotation_source = [1.0, 0.0, 0.0, 0.0]
    scale_list = []
    scale_source = [1.0, 1.0, 1.0]
    location_good = True
    rotation_good = True
    scale_good = True

    for selected_obj in context.selected_objects:
        if selected_obj is not []:
            obj = selected_obj.matrix_world.decompose()
            # Start a matrix count
            count = 0
            for vector in obj:
                # We will count from 1 to 3, every object will have a matrix of Location, Rotation, and Scale
                count += 1
                if count == 1:
                    # Add vector3 to Location list
                    for floatdata in vector:
                        location_list.append(floatdata)
                elif count == 2:
                    # Add vector4 to Rotation list
                    for floatdata in vector:
                        rotation_list.append(floatdata)
                else:
                    # Add vector3 to Scale list
                    for floatdata in vector:
                        scale_list.append(floatdata)
                    count = 0
                    
            # Lets look at the results and compair the sets
            if not compair_set(location_list, location_source):
                location_good = False
            elif not compair_set(rotation_list, rotation_source):
                rotation_good = False
            elif not compair_set(scale_list, scale_source):
                scale_good = False
            # Check if all are true or false
            check_transfroms_bools = [location_good, rotation_good, scale_good]
            if all(check_transfroms_bools):
                return True, location_list
            else:
                return False, location_list
        # Reset the list
        location_list = []
        rotation_list = []
        scale_list = []
        location_good = True
        rotation_good = True
        scale_good = True