# coding:utf-8
#!/usr/bin/python
"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
# -------------------------------------------------------------------------
import bpy
from bpy.props import EnumProperty
import shutil
from pathlib import Path
from . import ui

def check_selected():
    """!
    This function will check to see if the user has selected a object
    """
    selections = [obj.name for obj in bpy.context.selected_objects]
    if selections == []:
        ui.message_box("Nothing Selected....", "O3DE Tools", "ERROR")
        return False, "None"
    else:
        return True, selections

def selected_hierarchy_and_rig_animation():
    """!
    This function will check to see if an animation armature rig has been selected.
    """
    ob = bpy.ops.object
    
    for selected_obj in bpy.context.selected_objects:
        if selected_obj is not []:
            ob.select_grouped(extend=True, type='SIBLINGS')
            ob.select_grouped(extend=True, type='PARENT')
            ob.select_grouped(extend=True, type='CHILDREN_RECURSIVE')
            ob.select_grouped(extend=True, type='PARENT')

def valid_animation_selection():
    """!
    This function will check to see if user has selected a valid level in
    the hierarchy for exporting the selected mesh, armature rig and animation.
    """
    object_selections = [obj for obj in bpy.context.selected_objects]
    obj = object_selections[0]

    if obj.type == 'ARMATURE':
        bpy.types.Scene.export_good = False
        ui.message_box("You must select lowest level mesh of Armature.", "O3DE Tools", "ERROR")
    elif obj.children:
        bpy.types.Scene.export_good = False
        ui.message_box("You need to select base mesh level.", "O3DE Tools", "ERROR")
    else:
        bpy.types.Scene.export_good = True
        selected_hierarchy_and_rig_animation()

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

    for obj in selected:
        try:
            materials.append(obj.active_material.name)
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
        # Get the material
        material = bpy.data.materials[mat]
        # Loop through material node tree and get all the texture iamges
        for img in material.node_tree.nodes:
            if img.type == 'TEX_IMAGE':
                # Frist make sure the image is not packed inside blender
                if img.image.packed_file:
                    bpy.data.images[img.image.name].filepath = str(Path(texture_file_path).joinpath(img.image.name))
                    bpy.data.images[img.image.name].save()
                full_path = bpy.path.abspath(img.image.filepath, library=img.image.library)
                base_name = Path(full_path).name
                o3de_texture_path = Path(texture_file_path).joinpath(base_name)
                # Add to stored_image_source_paths to replace later
                if not check_file_paths_duplicate(full_path, o3de_texture_path): # We check first if its not already copied over.
                    bpy.types.Scene.stored_image_source_paths[img.image.name] = full_path
                    # Copy the image to Destination
                    try:
                        shutil.copyfile(full_path, o3de_texture_path)
                        # Find image and repath
                        bpy.data.images[img.image.name].filepath = str(o3de_texture_path)
                        # Save image to location
                        bpy.data.images[img.image.name].save()
                    except FileNotFoundError:
                        pass
                img.image.reload()

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
            project_directory = str(Path(bpy.types.Scene.selected_o3de_project_path).name)
            projectSelectionList.append((str(bpy.types.Scene.selected_o3de_project_path),
                project_directory, str(bpy.types.Scene.selected_o3de_project_path)))
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

def ReplaceStoredPaths():
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