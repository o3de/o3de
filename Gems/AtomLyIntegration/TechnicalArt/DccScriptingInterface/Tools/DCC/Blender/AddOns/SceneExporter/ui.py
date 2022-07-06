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
import webbrowser
import re
from bpy_extras.io_utils import ExportHelper
from bpy.types import Panel, Operator, PropertyGroup
from bpy.props import EnumProperty, StringProperty, BoolProperty, PointerProperty
from . import constants
from . import fbx_exporter
from . import o3de_utils
from . import utils

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
    """!
    This function will send to selected mesh to an O3DE Project Path.
    """
    bl_idname = "vin.o3defilexport"
    bl_label = "SENDFILES"

    def export_files(self, file_name):
        """!
        This function will export the selected as an .fbx to the current project path.
        @param file_name is the file name selected or string in export_file_name_o3de
        """
        # Add file ext
        file_name = f'{file_name}.fbx'
        fbx_exporter.fbx_file_exporter('', file_name)

    def execute(self, context):
        """!
        This function will check the current selected count and multi_file_export_o3de bool is True or False.
        If multi_file_export_o3de is True it will export each selected object as an .fbx file, if False it will
        export all objects selected as one .fbx.
        @param context defualt for this blender class
        """
        # Validate a selection
        valid_selection, selected_name = utils.check_selected()
        # Check if there are multi selections
        if len(selected_name) > 1:
            if bpy.types.Scene.multi_file_export_o3de:
                for obj_name in selected_name:
                    bpy.data.objects[obj_name].select_set(True)
                    # Remove some nasty invalid char
                    file_name = re.sub(r'\W+', '', obj_name)
                    # Export file
                    self.export_files(file_name)
            else:
                # Remove some nasty invalid char
                file_name = re.sub(r'\W+', '', bpy.context.scene.export_file_name_o3de)
                # Export file
                self.export_files(file_name)
        else:
            # Remove some nasty invalid char
            file_name = re.sub(r'\W+', '', bpy.context.scene.export_file_name_o3de)
            # Export file
            self.export_files(file_name)
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

    export_animation : BoolProperty(
        name="Keyframe Animation",
        description="Export with Keyframe Animation",
        default=False,
    )

    def execute(self, context):
        """!
        This function will select the Export Texture Option
        """
        bpy.types.Scene.export_textures_folder = self.export_textures_folder
        bpy.types.Scene.file_menu_animation_export = self.export_animation
        if self.export_animation:
            bpy.types.Scene.animation_export = constants.KEY_FRAME_ANIMATION
            utils.valid_animation_selection()
        fbx_exporter.fbx_file_exporter(self.filepath, Path(self.filepath).stem)
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
        layout.prop(context.scene, 'texture_options_list')
        
    def execute(self, context):
        """!
        This function will Update Export Option Bool
        """
        if context.scene.texture_options_list == '0':
            bpy.types.Scene.export_textures_folder = True
        elif context.scene.texture_options_list == '1':
            bpy.types.Scene.export_textures_folder = False
        else:
            bpy.types.Scene.export_textures_folder = None
        return {'FINISHED'}

class AnimationOptionsListDropDown(bpy.types.Operator):
    """!
    This Class is for the O3DE Export Animations UI List Drop Down
    """
    bl_label = "Animation Export Options"
    bl_idname = "wm.animationoptions"

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
        layout.prop(context.scene, 'animation_options_list')
        
    def execute(self, context):
        """!
        This function will Update Export Option Bool
        """
        if context.scene.animation_options_list == '0':
            bpy.types.Scene.animation_export = constants.NO_ANIMATION
        elif context.scene.animation_options_list == '1':
            bpy.types.Scene.animation_export = constants.KEY_FRAME_ANIMATION
            utils.valid_animation_selection()
        elif context.scene.animation_options_list == '2':
            bpy.types.Scene.animation_export = constants.MESH_AND_RIG
            utils.valid_animation_selection()
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
        wm = context.window_manager
        # Look at the o3de Engine Manifest
        o3de_projects, engine_is_installed = o3de_utils.LookatEngineManifest()
        # Validate a selection
        valid_selection, selected_name = utils.check_selected()
        # Validate selection bone names if any
        bone_names = utils.check_selected_bone_names()

        if engine_is_installed: # Checks to see if O3DE is installed
            row.operator("vin.wiki", text='O3DE Tools Wiki', icon="WORLD_DATA")
            installed_lable = layout.row()
            if not obj is None:
                installed_lable.label(text=f'OBJECTS({len(selected_name)}): {obj.name}')
            else:
                installed_lable.label(text='OBJECTS: None')
            # If more than 1 object is selected, let user choose export options
            if len(selected_name) > 1:
                multi_file_export_label = "Export Multi-Files" if wm.multi_file_export_toggle else "Export Single File"
                multi_file_export_button = layout.row()
                multi_file_export_button.prop(wm, "multi_file_export_toggle", text=multi_file_export_label, toggle=True)
                if wm.multi_file_export_toggle:
                    bpy.types.Scene.multi_file_export_o3de = True
                else:
                    bpy.types.Scene.multi_file_export_o3de = False
            else:
                bpy.types.Scene.multi_file_export_o3de = False
            # Let user choose a custom file name
            file_name_lable = layout.row()
            if not wm.multi_file_export_toggle:
                file_name_lable.label(text='Export File Name')
                file_name_export_input = self.layout.column(align = True)
                file_name_export_input.prop(context.scene, "export_file_name_o3de")
            # Let user choose a custom project path
            project_path_lable = layout.row()
            if not bpy.types.Scene.selected_o3de_project_path == '':
                project_path_lable.label(text=f'PROJECT: {Path(bpy.types.Scene.selected_o3de_project_path).name}')
            else:
                project_path_lable.label(text='PROJECT: None')
            # Let user choose animation export options
            animation_export_lable = layout.row()
            if not bpy.types.Scene.animation_export == constants.NO_ANIMATION:
                animation_export_lable.label(text=f'ANIMATION: {bpy.types.Scene.animation_export}', icon="POSE_HLT")
            else:
                animation_export_lable.label(text=f'ANIMATION: {bpy.types.Scene.animation_export}', icon="CANCEL")
                
            # This is the UI Export Texture Option Label
            texture_export_option_label = layout.row()
            if bpy.types.Scene.export_textures_folder:
                texture_export_option_label.label(text='Texture Export: (TF)', icon="FILEBROWSER")
            elif bpy.types.Scene.export_textures_folder is False:
                texture_export_option_label.label(text='Texture Export: (TWM)', icon="COPY_ID")
            elif bpy.types.Scene.export_textures_folder is None:
                texture_export_option_label.label(text='Texture Export: (MO)', icon="CANCEL")
            # This is the UI Porjects List
            o3de_projects_panel = layout.row()
            o3de_projects_panel.operator('wm.projectlist', text='O3DE Projects', icon="OUTLINER")
            # This is the UI Texture Export Options List
            texture_export_options_panel = layout.row()
            texture_export_options_panel.operator('wm.exportoptions', text='Texture Export Options', icon="OUTPUT")
            # This is the UI Animation Export Options List
            animation_export_options_panel = layout.row()
            animation_export_options_panel.operator('wm.animationoptions', text='Animation Export Options', icon="POSE_HLT")
            # This checks to see if we should enable the export button
            export_files_row = layout.row()
            export_ready_row = layout.row()
            if bpy.types.Scene.selected_o3de_project_path == '':
                export_files_row.enabled = False
                export_ready_row.label(text='No Project Selected.')
            elif not utils.check_if_valid_path(bpy.types.Scene.selected_o3de_project_path):
                export_files_row.enabled = False
                export_ready_row.label(text='Project Path Not Found.')
            elif not bpy.types.Scene.export_good_o3de:
                export_files_row.enabled = False
                export_ready_row.label(text='Not Ready for Export.')
            elif not valid_selection:
                export_files_row.enabled = False
                export_ready_row.label(text='Nothing Selected.')
            elif bone_names == False:
                export_files_row.enabled = False
                export_ready_row.label(text='Invalid Char in Bone Names.')
            else:
                export_files_row.enabled = True
                
            # This is the export button
            export_files_row.operator('vin.o3defilexport', text='EXPORT TO O3DE', icon="BLENDER")
        else:
            # If O3DE is not installed we tell the user
            row.operator("vin.wiki", text='O3DE Tools Wiki', icon="WORLD_DATA")
            not_installed = layout.row()
            not_installed.label(text='O3DE Needs to be installed')

