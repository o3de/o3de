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
"""! @brief This is the entry point for the DCCsi Launcher. Come up with a better description """

##
# @file main.py
#
# @brief The Blender O3DE Scene exporter is a convenience tool for one click
# exporter from Blender to O3DE.
#
# @section This DCCsi tool is non-destructive to your scene, as it will export
# selected copies of your mesh and textures, re-path texture links to your mesh
# during file export. You can export selected static meshes or rigged animated
# meshes within the capabilities of the .FBX format and O3DE actor entity.
#
# You have options to export textures with the mesh, or with the mesh within
# a sub folder "/textures". This will work for Blender Scenes with or without
# packed textures.
#
# Hardware and Software Requirements:
#
# Support for Blender 3.+ Windows 10/11 64-bit version
# O3DE Stable 21.11 Release Windows 10/11 64-bit version
#
# @section Launcher Notes
# - Comments are Doxygen compatible

# To DO: Check for invalid Bone.oo1 names. Fix the not selected bug. No we need Bone Ends?

bl_info = {
    "name": "O3DE_DCCSI_BLENDER_SCENE_EXPORTER",
    "author": "shawstar@amazon",
    "version": (1, 3),

    "blender": (3, 00, 0),
    "location": "",
    "description": "Export Scene Assets to O3DE",
    "warning": "",
    "doc_url": "",
    "category": "Import-Export",
}  # This is needed for Blender Plugin

from typing import Any
import bpy
from bpy.props import EnumProperty
import sys
from pathlib import Path
# Needed to import custom scripts in Blender Python
directory = Path.cwd()
sys.path += [str(directory)]

from . import o3de_utils
from . import ui
from . import constants


def register():
    """! This is the function that will register Classes and Global Vars for this plugin
    """
    bpy.utils.register_class(ui.O3deTools)
    bpy.utils.register_class(ui.WikiButton)
    bpy.utils.register_class(ui.ExportFiles)
    bpy.utils.register_class(ui.ProjectsListDropDown)
    bpy.utils.register_class(ui.SceneExporterFileMenu)
    bpy.utils.register_class(ui.ExportOptionsListDropDown)
    bpy.utils.register_class(ui.AnimationOptionsListDropDown)
    bpy.types.Scene.export_file_name_o3de = bpy.props.StringProperty(
        name="",
        description="Export File Name",
        default="o3de_export"
    )
    bpy.types.TOPBAR_MT_file_export.append(ui.file_export_menu_add)  # Blender Specific Class and Naming Convention. 
    bpy.types.Scene.selected_o3de_project_path = ''
    bpy.types.Scene.export_textures_folder = True
    bpy.types.Scene.animation_export = constants.NO_ANIMATION
    bpy.types.Scene.file_menu_animation_export = False
    bpy.types.Scene.export_good_o3de = True
    bpy.types.Scene.multi_file_export_o3de = False
    bpy.types.Scene.stored_image_source_paths = {}
    bpy.types.Scene.o3de_projects_list = EnumProperty(items=o3de_utils.build_projects_list(), name='')
    bpy.types.Scene.texture_options_list = EnumProperty(items=constants.EXPORT_LIST_OPTIONS, name='', default='0')
    bpy.types.Scene.animation_options_list = EnumProperty(items=constants.ANIMATION_LIST_OPTIONS, name='', default='0')
    bpy.types.WindowManager.multi_file_export_toggle = bpy.props.BoolProperty()


def unregister():
    """! This is the function that will unregister Classes and Global Vars for this plugin
    """
    bpy.utils.unregister_class(ui.O3deTools)
    bpy.utils.unregister_class(ui.WikiButton)
    bpy.utils.unregister_class(ui.ExportFiles)
    bpy.utils.unregister_class(ui.ProjectsListDropDown)
    bpy.utils.unregister_class(ui.SceneExporterFileMenu)
    bpy.utils.unregister_class(ui.ExportOptionsListDropDown)
    bpy.utils.unregister_class(ui.AnimationOptionsListDropDown)
    bpy.types.TOPBAR_MT_file_export.remove(ui.file_export_menu_add)  # Blender Specific Class and Naming Convention. 
    del bpy.types.Scene.export_file_name_o3de
    del bpy.types.Scene.selected_o3de_project_path
    del bpy.types.Scene.o3de_projects_list
    del bpy.types.Scene.texture_options_list
    del bpy.types.Scene.animation_options_list
    del bpy.types.Scene.export_good_o3de
    del bpy.types.Scene.multi_file_export_o3de
    del bpy.types.Scene.export_textures_folder
    del bpy.types.Scene.animation_export
    del bpy.types.Scene.file_menu_animation_export
    del bpy.types.Scene.stored_image_source_paths
    del bpy.types.WindowManager.multi_file_export_toggle


if __name__ == "__main__":
    register()
