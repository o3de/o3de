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
from pathlib import Path
from . import utils
from . import o3de_utils
from . import constants

def amimation_export_options():
    """!
    This function will return the selected animation options from the O3DE Dropdown
    Animation Export Options.
    """
    # Animation Export Options:
    # Set Default Values
    bake_anim_option = None
    bake_anim_use_all_bones = None
    bake_anim_use_nla_strips_option = None
    bake_anim_use_all_actions_option = None
    bake_anim_force_startend_keying_option = None

    if bpy.types.Scene.animation_export == constants.NO_ANIMATION:
        bake_anim_option = False
        bake_anim_use_all_bones = False
        bake_anim_use_nla_strips_option = False
        bake_anim_use_all_actions_option = False
        bake_anim_force_startend_keying_option = False
        bpy.types.Scene.file_menu_animation_export = False
        return bake_anim_option, bake_anim_use_all_bones, bake_anim_use_nla_strips_option, bake_anim_use_all_actions_option, bake_anim_force_startend_keying_option
    elif bpy.types.Scene.animation_export == constants.KEY_FRAME_ANIMATION:
        # Set Animation Options
        bake_anim_option = True
        bake_anim_use_all_bones = True
        bake_anim_use_nla_strips_option = True
        bake_anim_use_all_actions_option = True
        bake_anim_force_startend_keying_option = True
        bpy.types.Scene.file_menu_animation_export = True
        return bake_anim_option, bake_anim_use_all_bones, bake_anim_use_nla_strips_option, bake_anim_use_all_actions_option, bake_anim_force_startend_keying_option
    elif bpy.types.Scene.animation_export == constants.MESH_AND_RIG:
        # Set Animation Options
        bake_anim_option = False
        bake_anim_use_all_bones = False
        bake_anim_use_nla_strips_option = False
        bake_anim_use_all_actions_option = False
        bake_anim_force_startend_keying_option = False
        bpy.types.Scene.file_menu_animation_export = False
        return bake_anim_option, bake_anim_use_all_bones, bake_anim_use_nla_strips_option, bake_anim_use_all_actions_option, bake_anim_force_startend_keying_option
    elif bpy.types.Scene.animation_export == constants.SKIN_ATTACHMENT:
        # Set Animation Options
        bake_anim_option = False
        bake_anim_use_all_bones = False
        bake_anim_use_nla_strips_option = False
        bake_anim_use_all_actions_option = False
        bake_anim_force_startend_keying_option = False
        bpy.types.Scene.file_menu_animation_export = False
        return bake_anim_option, bake_anim_use_all_bones, bake_anim_use_nla_strips_option, bake_anim_use_all_actions_option, bake_anim_force_startend_keying_option

    if bpy.types.Scene.file_menu_animation_export:
        bake_anim_option = True
        bake_anim_use_all_bones = True
        bake_anim_use_nla_strips_option = True
        bake_anim_use_all_actions_option = True
        bake_anim_force_startend_keying_option = True
        return bake_anim_option, bake_anim_use_all_bones, bake_anim_use_nla_strips_option, bake_anim_use_all_actions_option, bake_anim_force_startend_keying_option
    else:
        bake_anim_option = False
        bake_anim_use_all_bones = False
        bake_anim_use_nla_strips_option = False
        bake_anim_use_all_actions_option = False
        bake_anim_force_startend_keying_option = False
        return bake_anim_option, bake_anim_use_all_bones, bake_anim_use_nla_strips_option, bake_anim_use_all_actions_option, bake_anim_force_startend_keying_option

def fbx_file_exporter(fbx_file_path, file_name):
    """!
    This function will send to selected .FBX to an O3DE Project Path
    @param fbx_file_path this is the o3de project path where the selected meshe(s)
    will be exported as an .fbx
    @param file_name A custom file name string
    """
    # Export file path Var
    export_file_path = ''
    # Validate a selection
    valid_selection, selected_name = utils.check_selected()

    # FBX Exporter
    if valid_selection:
        if fbx_file_path == '':
            # Build new path, check to see if this is a custom or tool made path
            # and if has the Assets Directory.
            asset_path = Path(bpy.types.Scene.selected_o3de_project_path).joinpath('Assets')
            if Path(asset_path).exists():
                # TOOL MENU EXPORT
                export_file_path = Path(bpy.types.Scene.selected_o3de_project_path).joinpath('Assets', file_name)
                # Clone Texture images and Repath images before export
                file_menu_export = False
                if not bpy.types.Scene.export_textures_folder is None:
                    utils.clone_repath_images(file_menu_export, bpy.types.Scene.selected_o3de_project_path, o3de_utils.build_projects_list())
            else:
                # WAS ONCE FILE MENU EXPORT
                export_file_path = Path(bpy.types.Scene.selected_o3de_project_path).joinpath(file_name)
                # Clone Texture images and Repath images before export
                file_menu_export = None # This is because it was first exported by the file menu export
                if not bpy.types.Scene.export_textures_folder is None:
                    utils.clone_repath_images(file_menu_export, bpy.types.Scene.selected_o3de_project_path, o3de_utils.build_projects_list())
        else:
            # Build new path
            export_file_path = fbx_file_path
            source_file_path = Path(fbx_file_path) # Covert string to path
            bpy.types.Scene.selected_o3de_project_path = Path(source_file_path.parent)
            # Clone Texture images and Repath images before export
            file_menu_export = True
            if not bpy.types.Scene.export_textures_folder is None:
                utils.clone_repath_images(file_menu_export, source_file_path, o3de_utils.build_projects_list())
                # Currently the Blender FBX Export API is not support use_triangles, we will need to use a modifier at export then remove when done.
        if bpy.types.Scene.convert_mesh_to_triangles:
            utils.add_remove_modifier("TRIANGULATE", True)
        # Lets get the Animation Options
        bake_anim_option, bake_anim_use_all_bones, bake_anim_use_nla_strips_option, bake_anim_use_all_actions_option, bake_anim_force_startend_keying_option = amimation_export_options()
        # Main Blender FBX API exporter
        bpy.ops.export_scene.fbx(
            filepath=str(export_file_path),
            check_existing=False,
            filter_glob='*.fbx',
            use_selection=True,
            use_active_collection=False,
            global_scale=1.0,
            apply_unit_scale=True,
            apply_scale_options='FBX_SCALE_UNITS',
            use_space_transform=True,
            bake_space_transform=False,
            object_types={'ARMATURE', 'CAMERA', 'EMPTY', 'LIGHT', 'MESH', 'OTHER'},
            use_mesh_modifiers=True,
            use_mesh_modifiers_render=True,
            mesh_smooth_type='OFF',
            use_subsurf=False,
            use_mesh_edges=False,
            use_tspace=False,
            use_custom_props=False,
            add_leaf_bones=False,
            primary_bone_axis='Y',
            secondary_bone_axis='X',
            use_armature_deform_only=False,
            armature_nodetype='NULL',
            bake_anim=bake_anim_option,
            bake_anim_use_all_bones=bake_anim_use_all_bones,
            bake_anim_use_nla_strips=bake_anim_use_nla_strips_option,
            bake_anim_use_all_actions=bake_anim_use_all_actions_option,
            bake_anim_force_startend_keying=bake_anim_force_startend_keying_option,
            bake_anim_step=1.0,
            bake_anim_simplify_factor=1.0,
            path_mode='AUTO',
            embed_textures=True,
            batch_mode='OFF',
            use_batch_own_dir=False,
            use_metadata=True,
            axis_forward='-Z',
            axis_up='Y')
        
        # If we added a Triangulate modifier, lets remove it now.
        if bpy.types.Scene.convert_mesh_to_triangles:
            utils.add_remove_modifier("Triangulate", False)
        # Show export status
        bpy.types.Scene.pop_up_notes = f'{file_name} Exported!'
        bpy.ops.message.popup('INVOKE_DEFAULT')
        if not bpy.types.Scene.export_textures_folder is None:
            utils.replace_stored_paths()
