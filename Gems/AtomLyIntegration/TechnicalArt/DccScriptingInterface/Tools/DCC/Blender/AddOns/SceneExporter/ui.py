# coding:utf-8
#!/usr/bin/python
"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
# -------------------------------------------------------------------------
import bpy
from pathlib import Path
import webbrowser
from bpy_extras.io_utils import ExportHelper
from bpy.types import Panel, Operator, PropertyGroup
from bpy.props import EnumProperty, StringProperty, BoolProperty, PointerProperty
from . import constants
from . import fbx_exporter
from . import o3de_utils
from . import ui

def message_box(message = "", title = "Message Box", icon = 'LIGHT'):
    """!
    This function will show a messagebox to inform user.
    @param message This is the message box main string message
    @param title This is the title of the message box string
    @param icon This is the Blender icon used in the message box gui
    """
    def draw(self, context):
        """!
        This function draws this gui element in Blender UI
        """
        self.layout.label(text = message)
    bpy.context.window_manager.popup_menu(draw, title = title, icon = icon)
class WikiButton(bpy.types.Operator):
    """!
    This Class is for the UI Wiki Button
    """
    bl_idname = "vin.wiki"
    bl_label = "O3DE Github Wiki"

    def execute(self, context):
        """!
        This function will open a web browser window.
        """
        webbrowser.open(constants.WIKI_URL)
        return{'FINISHED'}

class ExportFiles(bpy.types.Operator):
    bl_idname = "vin.o3defilexport"
    bl_label = "SENDFILES"

    def execute(self, contex):
        """!
        This function will send to selected GLB to an O3DE Project Path.
        """
        fbx_exporter.fbx_file_exporter('')
        return{'FINISHED'}

class ProjectsListDropDown(bpy.types.Operator):
    """!
    This Class is for the O3DE Projects UI List Drop Down.
    """
    bl_label = "Project List Dropdown"
    bl_idname = "wm.projectlist"

    def invoke(self, context, event):
        """!
        This function will invoke the blender windowe manager
        """
        window_manager = context.window_manager
        return window_manager.invoke_props_dialog(self)

    def draw(self, context):
        """!
        This function draws this gui element in Blender UI
        """
        layout = self.layout
        layout.prop(context.scene, 'o3de_projects_list')
        
    def execute(self, context):
        """!
        This will update the UI with the current o3de Project Title.
        """
        bpy.types.Scene.selected_o3de_project_path = context.scene.o3de_projects_list
        return {'FINISHED'}

class SceneExporterFileMenu(Operator, ExportHelper):
    """!
    This Class will export the 3d model as well
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
    export_textures_folder : BoolProperty(
        name="Export Textures in a textures folder",
        description="Export Textures in textures folder",
        default=True,
    )

    def execute(self, context):
        """!
        This function will select the Export Texture Option
        """
        bpy.types.Scene.export_textures_folder = self.export_textures_folder
        fbx_exporter.fbx_file_exporter(self.filepath)
        return{'FINISHED'}

class ExportOptionsListDropDown(bpy.types.Operator):
    """!
    This Class is for the O3DE Export Options UI List Drop Down
    """
    bl_label = "Texture Export Folder"
    bl_idname = "wm.exportoptions"

    def invoke(self, context, event):
        """!
        This function will invoke the blender windowe manager
        """
        window_manager = context.window_manager
        return window_manager.invoke_props_dialog(self)
        
    def draw(self, context):
        """!
        This function draws this gui element in Blender UI
        """
        layout = self.layout
        layout.prop(context.scene, 'export_options_list')
        
    def execute(self, context):
        """!
        This function will Update Export Option Bool
        """
        if context.scene.export_options_list == '0':
            bpy.types.Scene.export_textures_folder = True
        elif context.scene.export_options_list == '1':
            bpy.types.Scene.export_textures_folder = False
        else:
            bpy.types.Scene.export_textures_folder = None
        return {'FINISHED'}

def file_export_menu_add(self, context):
    """!
    This Function will add the Export to O3DE to the file menu Export
    """
    self.layout.operator(SceneExporterFileMenu.bl_idname, text="Export to O3DE")

class O3deTools(Panel):
    """!
    This is the Blender UI Panel O3DE Tools Tab
    """
    bl_idname = "O3DE_Tools"
    bl_space_type = 'VIEW_3D'
    bl_region_type = 'UI'
    bl_label = 'O3DE Tools'
    bl_context = 'objectmode'
    bl_category = 'O3DE'

    def draw(self, context):
        """!
        This function draws this gui element in Blender UI. We will look at the Look at the
        o3de engine manifest to see if o3de is currently install and gather the project paths
        in a list drop down.
        """
        layout = self.layout
        obj = context.object
        row = layout.row()
        installed_lable = layout.row()
        # Look at the o3de Engine Manifest
        o3de_projects, engine_is_installed = o3de_utils.LookatEngineManifest()

        if engine_is_installed: # Checks to see if O3DE is installed

            if not obj is None:
                installed_lable.label(text='MESH(S): ' + obj.name)
            else:
                installed_lable.label(text='MESH(S): None')

            row.operator("vin.wiki", text='O3DE Tools Wiki', icon="WORLD_DATA")

            project_path_lable = layout.row()
            if not bpy.types.Scene.selected_o3de_project_path == '':
                project_path_lable.label(text=f'PROJECT: {Path(bpy.types.Scene.selected_o3de_project_path).name}')
            else:
                project_path_lable.label(text='PROJECT: ')
            # This is the UI Export Option Label
            export_option_label = layout.row()
            if bpy.types.Scene.export_textures_folder:
                export_option_label.label(text='Export Option: (TF)')
            elif bpy.types.Scene.export_textures_folder is False:
                export_option_label.label(text='Export Option: (TWM)')
            elif bpy.types.Scene.export_textures_folder is None:
                export_option_label.label(text='Export Option: (MO)')
            # This is the UI Porjects List
            o3de_projects_panel = layout.row()
            o3de_projects_panel.operator('wm.projectlist', text='O3DE Projects', icon="OUTLINER")
            # This is the UI Export Options List
            o3de_export_options_panel = layout.row()
            o3de_export_options_panel.operator('wm.exportoptions', text='Export Options', icon="OUTPUT")
            # This checks to see if we should enable the export button
            ui.ExportFiles = layout.row()
            if bpy.types.Scene.selected_o3de_project_path == '':
                ui.ExportFiles.enabled = False
            else:
                ui.ExportFiles.enabled = True
            # This is the export button
            ui.ExportFiles.operator('vin.o3defilexport', text='EXPORT TO O3DE', icon="BLENDER")
        else:
            # If O3DE is not installed we tell the user
            row.operator("vin.wiki", text='O3DE Tools Wiki', icon="WORLD_DATA")
            not_installed = layout.row()
            not_installed.label(text='O3DE Needs to be installed')