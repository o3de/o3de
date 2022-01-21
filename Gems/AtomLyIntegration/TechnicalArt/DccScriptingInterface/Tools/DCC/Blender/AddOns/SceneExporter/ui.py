# coding:utf-8
#!/usr/bin/python
"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
# -------------------------------------------------------------------------
import bpy
import os
import webbrowser
from bpy_extras.io_utils import ExportHelper
from bpy.types import Panel, Operator, PropertyGroup
from bpy.props import EnumProperty, StringProperty, BoolProperty, PointerProperty
import fbx_exporter
import o3de
import ui

def MessageBox(message = "", title = "Message Box", icon = 'LIGHT'):
    """
    This function will show a messagebox to inform user.
    """
    def draw(self, context):
        self.layout.label(text = message)
    
    bpy.context.window_manager.popup_menu(draw, title = title, icon = icon)

class WikiButton(bpy.types.Operator):
    """
    This Class is for the UI Wiki Button
    """
    bl_idname = "vin.wiki"
    bl_label = "O3DE Github Wiki"

    def execute(self, context):
        """
        This function will open a web browser window
        """
        webbrowser.open('https://github.com/o3de/o3de/wiki/')
        return{'FINISHED'}

class ExportFiles(bpy.types.Operator):
    bl_idname = "vin.o3defilexport"
    bl_label = "SENDFILES"

    def execute(self, contex):
        """
        This function will send to selected GLB to an O3DE Project Path
        """
        fbx_exporter.fbxFileExporter('')
        return{'FINISHED'}

class Projects_OT_ListDropDown(bpy.types.Operator):
    """
    This Class is for the O3DE Projects UI List Drop Down
    """
    bl_label = "Project List Dropdown"
    bl_idname = "wm.projectlist"
    
    def invoke(self, context, event):
        wm = context.window_manager
        return wm.invoke_props_dialog(self)
        
    def draw(self, context):
        layout = self.layout
        layout.prop(context.scene, 'projectsWorkingList')
        
    def execute(self, context):
        bpy.types.Scene.selectedo3deProjectPath = context.scene.projectsWorkingList
        return {'FINISHED'}

class SceneExporterFileMenu(Operator, ExportHelper):
    """
    This function will export the 3d model as well
    as textures in the user selected path.
    """
    bl_idname = "O3DE_FileExport"  # important since its how bpy.ops.import_test.some_data is constructed
    bl_idname = "export_model.mesh_data"  # important since its how bpy.ops.import_test.some_data is constructed
    bl_label = "Export to O3DE"

    # ExportHelper mixin class uses this
    filename_ext = ".fbx"

    filter_glob: StringProperty(
        default="*.fbx",
        options={'HIDDEN'},
        maxlen=255,  # Max internal buffer length, longer would be clamped.
    )
    # Extra options
    exportTexturesFolder : BoolProperty(
        name="Export Textures in a textures folder",
        description="Export Textures in textures folder",
        default=True,
    )

    def execute(self, context):
        bpy.types.Scene.exportInTextureFolder = self.exportTexturesFolder
        fbx_exporter.fbxFileExporter(self.filepath)
        return{'FINISHED'}

class ExportOptions_OT_ListDropDown(bpy.types.Operator):
    """
    This Class is for the O3DE Export Options UI List Drop Down
    """
    bl_label = "Texture Export Folder"
    bl_idname = "wm.exportoptions"

    def invoke(self, context, event):
        wm = context.window_manager
        return wm.invoke_props_dialog(self)
        
    def draw(self, context):
        layout = self.layout
        layout.prop(context.scene, 'exportOptionsList')
        
    def execute(self, context):
        # Update Export Option Bool
        if context.scene.exportOptionsList == '0':
            bpy.types.Scene.exportInTextureFolder = True
        elif context.scene.exportOptionsList == '1':
            bpy.types.Scene.exportInTextureFolder = False
        else:
            bpy.types.Scene.exportInTextureFolder = None
        return {'FINISHED'}

def FileExportMenuAdd(self, context):
    """
    This Function will add the Export to O3DE to the file menu Export
    """
    self.layout.operator(SceneExporterFileMenu.bl_idname, text="Export to O3DE")

class O3deTools(Panel):
    bl_idname = "O3DE_Tools"
    bl_space_type = 'VIEW_3D'
    bl_region_type = 'UI'
    bl_label = 'O3DE Tools'
    bl_context = 'objectmode'
    bl_category = 'O3DE'

    def draw(self, context):
        layout = self.layout
        obj = context.object
        scene = context.scene
        row = layout.row()
        mainLable = layout.row()

        o3deDefaultProjectsFolder, o3deProjects, engineIsInstalled = o3de.LookatEngineManifest()

        if engineIsInstalled: # Checks to see if O3DE is installed

            if not obj == None:
                mainLable.label(text='MESH(S): ' + obj.name)
            else:
                mainLable.label(text='MESH(S): None')

            row.operator("vin.wiki", text='O3DE Tools Wiki', icon="WORLD_DATA")

            projectPathLable = layout.row()
            if not bpy.types.Scene.selectedo3deProjectPath == '':
                projectPathLable.label(text='PROJECT: {}'.format(os.path.basename(bpy.types.Scene.selectedo3deProjectPath)))
            else:
                projectPathLable.label(text='PROJECT: ')
            # This is the UI Export Option Label
            exportOptionLabel = layout.row()
            if bpy.types.Scene.exportInTextureFolder:
                exportOptionLabel.label(text='Export Option: (TF)')
            elif bpy.types.Scene.exportInTextureFolder == False:
                exportOptionLabel.label(text='Export Option: (TWM)')
            elif bpy.types.Scene.exportInTextureFolder == None:
                exportOptionLabel.label(text='Export Option: (MO)')
            # This is the UI Porjects List
            o3deProjectsPanel = layout.row()
            o3deProjectsPanel.operator('wm.projectlist', text='O3DE Projects', icon="OUTLINER")
            # This is the UI Export Options List
            o3deexportOptionsPanel = layout.row()
            o3deexportOptionsPanel.operator('wm.exportoptions', text='Export Options', icon="OUTPUT")
            # This checks to see if we should enable the export button
            ui.ExportFiles = layout.row()
            if bpy.types.Scene.selectedo3deProjectPath == '':
                ui.ExportFiles.enabled = False
            else:
                ui.ExportFiles.enabled = True
            # This is the export button
            ui.ExportFiles.operator('vin.o3defilexport', text='EXPORT TO O3DE', icon="BLENDER")
        else:
            # If O3DE is not installed we tell the user
            row.operator("vin.wiki", text='O3DE Tools Wiki', icon="WORLD_DATA")
            notInstalled = layout.row()
            notInstalled.label(text='O3DE Needs to be installed')