# coding:utf-8
#!/usr/bin/python
"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
# -------------------------------------------------------------------------
"""! @brief This is the entry point for the DCCsi Launcher. Come up with a better description """

##
# @file main.py
#
# @brief The Blender O3DE Scene exporter is a convenience tool
#  for one click exporter from Blender to O3DE.
#
# @section This DCCsi tool is non-destructive to your scene,
# as it will export selected copies of your mesh and textures,
# re-path texture links to your mesh during file export.
# You can export selected static meshes or rigged animated
# meshes within the capabilities of the .FBX format and O3DE
# actor entity.
# You have options to export textures with the mesh,
# or with the mesh within a sub folder ‘textures’.
# This will work for Blender Scenes with or without packed textures.
#
# @section Launcher Notes
# - Comments are Doxygen compatible

bl_info = {
    "name": "O3DE_DCCSI_BLENDER_SCENE_EXPORTER",
    "author": "shawstar@amazon",
    "version": (1, 0),
    "blender": (3, 00, 0),
    "location": "",
    "description": "Export Scene Assets to O3DE",
    "warning": "",
    "doc_url": "",
    "category": "Import-Export",
} # This is needed for Blender Plugin

from typing import Any
from . import auto_load
import bpy
from bpy.props import EnumProperty
import sys
from pathlib import Path

# Needed to import custom scripts in Blender Python
directory = Path.cwd()
sys.path += [str(directory)]

import o3de_utils
import ui
import constants

auto_load.init()

def register():
    """! This is the function that will register Classes and Global Vars for this plugin
    """
    auto_load.register()
    bpy.types.TOPBAR_MT_file_export.append(ui.file_export_menu_add) # Blender Specific Class and Naming Convention. 
    bpy.types.Scene.selected_o3de_project_path = ''
    bpy.types.Scene.export_textures_folder = True
    bpy.types.Scene.stored_image_source_paths = {}
    bpy.types.Scene.o3de_projects_list = EnumProperty(items=o3de_utils.build_projects_list(), name='')
    bpy.types.Scene.export_options_list = EnumProperty(items=constants.EXPORT_LIST_OPTIONS, name='', default='0')

def unregister():
    """! This is the function that will unregister Classes and Global Vars for this plugin
    """
    auto_load.unregister()
    bpy.types.TOPBAR_MT_file_export.remove(ui.file_export_menu_add) # Blender Specific Class and Naming Convention. 
    del bpy.types.Scene.selected_o3de_project_path
    del bpy.types.Scene.o3de_projects_list
    del bpy.types.Scene.export_options_list
    del bpy.types.Scene.export_textures_folder
    del bpy.types.Scene.stored_image_source_paths

if __name__ == "__main__":
    register()
