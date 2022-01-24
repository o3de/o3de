# coding:utf-8
#!/usr/bin/python
"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
# -------------------------------------------------------------------------

bl_info = {
    "name": "O3DE_DCCSI_BLENDER_SCENE_EXPORTER",
    "author": "shawstar@amazon",
    "version": (1, 0),
    "blender": (2, 80, 0),
    "location": "",
    "description": "Export Scene Assets to O3DE",
    "warning": "",
    "doc_url": "",
    "category": "Import-Export",
}

from typing import Any
from . import auto_load
import bpy
from bpy.props import EnumProperty
import os
import sys

# Needed to import custom scripts in Blender Python
directory = os.getcwd()
sys.path += [directory]

import o3de_utils
import ui

# Const
EXPORT_LIST_OPTIONS = ( ( ('0', 'Selected with in texture folder', 'Export selected meshes with textures in a texture folder.'),
    ('1', 'Selected with Textures in same folder', 'Export selected meshes with textures in the same folder'),
    ('2', 'Only Meshes', 'Only export the selected meshes, no textures')
    ))

auto_load.init()

def register():
    auto_load.register()
    bpy.types.TOPBAR_MT_file_export.append(ui.FileExportMenuAdd)
    bpy.types.Scene.selectedo3deEngine = ''
    bpy.types.Scene.selectedo3deProjectPath = ''
    bpy.types.Scene.exportInTextureFolder = True
    bpy.types.Scene.storedImageSourcePathsDict = {}
    bpy.types.Scene.projectsWorkingList = EnumProperty(items=o3de_utils.BuildProjectsList(), name='')
    bpy.types.Scene.exportOptionsList = EnumProperty(items=EXPORT_LIST_OPTIONS, name='', default='0')

def unregister():
    auto_load.unregister()
    bpy.types.TOPBAR_MT_file_export.remove(ui.FileExportMenuAdd)
    del bpy.types.Scene.selectedo3deEngine
    del bpy.types.Scene.selectedo3deProjectPath
    del bpy.types.Scene.projectsWorkingList
    del bpy.types.Scene.exportOptionsList
    del bpy.types.Scene.exportInTextureFolder
    del bpy.types.Scene.storedImageSourcePathsDict

if __name__ == "__main__":
    register()
