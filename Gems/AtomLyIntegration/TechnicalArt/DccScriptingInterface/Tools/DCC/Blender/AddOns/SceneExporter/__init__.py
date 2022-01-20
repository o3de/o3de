
bl_info = {
    "name": "O3DE_Scene_Exporter",
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
from bpy.types import Panel, Operator, PropertyGroup
from bpy.props import EnumProperty, StringProperty, BoolProperty, PointerProperty
from bpy_extras.io_utils import ExportHelper
import webbrowser
import os
import json
import re
import shutil

# Const
USER_NAME = os.getenv('username')
DEFAULT_SDK_MANIFEST_PATH = os.path.join('C:\\', 'Users', '{}', '.o3de', 'o3de_manifest.json').format(USER_NAME)
EXPORT_LIST_OPTIONS = ( ( ('0', 'Selected with in texture folder', 'Export selected meshes with textures in a texture folder.'),
    ('1', 'Selected with Textures in same folder', 'Export selected meshes with textures in the same folder'),
    ('2', 'Only Meshes', 'Only export the selected meshes, no textures')
    ))

# Optional Arguments for file menu exporter
fbxPath = ''

auto_load.init()

def CheckSelected():
    """
    This function will check to see if the user has selected a object
    """
    selections = [obj.name for obj in bpy.context.selected_objects]
    if selections == []:
        MessageBox("Nothing Selected....", "O3DE Tools", "ERROR")
        return False, "None"
    else:
        return True, selections

def GetSelectedMaterials():
    """
    This function will check the selected mesh and find material are connected
    then build a material list for selected.
    """
    materials = []

    for obj in bpy.context.selected_objects:
        try:
            materials.append(obj.active_material.name)
        except AttributeError:
            pass
    return materials


def MessageBox(message = "", title = "Message Box", icon = 'LIGHT'):
    """
    This function will show a messagebox to inform user.
    """
    def draw(self, context):
        self.layout.label(text = message)

    bpy.context.window_manager.popup_menu(draw, title = title, icon = icon)

def LookatEngineManifest():
    """
    This function will look at your O3DE engine manifest JSON
    """
    try:
        with open(DEFAULT_SDK_MANIFEST_PATH, "r") as data_file:
            data = json.load(data_file)
            # Look at manifest defualt project
            o3deDefaultProjectsFolder = data['default_projects_folder']
            # Look at manifest projects
            o3deProjects = data['projects']
            # Send if o3de is not installed
            bpy.types.Scene.engineIsInstalled = True
            return o3deDefaultProjectsFolder, o3deProjects
    except:
        o3deDefaultProjectsFolder = ''
        o3deProjects = ['']
        bpy.types.Scene.engineIsInstalled = False
        return o3deDefaultProjectsFolder, o3deProjects

def fbxFileExporter(fbxfilepath = fbxPath):
    """
    This function will send to selected .FBX to an O3DE Project Path
    """
    exportFilePath = ''
    # Validate a selection
    validSelection, selectedName = CheckSelected()
    # Remove some nasty invalid char
    filename = re.sub(r'\W+', '', selectedName[0])
    # FBX Exporter
    if validSelection:
        if fbxfilepath == '':
            # Build new path, check to see if this is a custom or tool made path
            # and if has the Assets Directory.
            if os.path.exists(os.path.join(bpy.types.Scene.selectedo3deProjectPath, 'Assets')):
                # TOOL MENU EXPORT
                exportFilePath = os.path.join(bpy.types.Scene.selectedo3deProjectPath, 'Assets', '{}{}'.format(filename, '.fbx'))
                # Clone Texture images and Repath images before export
                fileMenuExport = False
                if not bpy.types.Scene.exportInTextureFolder == None:
                    CloneAndRepathImages(fileMenuExport, bpy.types.Scene.selectedo3deProjectPath)
            else:
                # WAS ONCE FILE MENU EXPORT
                exportFilePath = os.path.join(bpy.types.Scene.selectedo3deProjectPath, '{}{}'.format(filename, '.fbx'))
                # Clone Texture images and Repath images before export
                fileMenuExport = None # This is because it was first exported by the file menu export
                if not bpy.types.Scene.exportInTextureFolder == None:
                    CloneAndRepathImages(fileMenuExport, bpy.types.Scene.selectedo3deProjectPath)
        else:
            # Build new path
            exportFilePath = fbxfilepath
            bpy.types.Scene.selectedo3deProjectPath = os.path.dirname(fbxfilepath)
            # Clone Texture images and Repath images before export
            fileMenuExport = True
            if not bpy.types.Scene.exportInTextureFolder is None:
                CloneAndRepathImages(fileMenuExport, fbxfilepath)

        bpy.ops.export_scene.fbx(
            filepath=exportFilePath,
            check_existing=False,
            filter_glob='*.fbx',
            use_selection=True,
            use_active_collection=False,
            global_scale=1.0,
            apply_unit_scale=True,
            apply_scale_options='FBX_SCALE_NONE',
            use_space_transform=True,
            bake_space_transform=False,
            object_types={'ARMATURE', 'CAMERA', 'EMPTY', 'LIGHT', 'MESH', 'OTHER'},
            use_mesh_modifiers=True, use_mesh_modifiers_render=True,
            mesh_smooth_type='OFF',
            use_subsurf=False,
            use_mesh_edges=False,
            use_tspace=False,
            use_custom_props=False,
            add_leaf_bones=True,
            primary_bone_axis='Y',
            secondary_bone_axis='X',
            use_armature_deform_only=False,
            armature_nodetype='NULL',
            bake_anim=True,
            bake_anim_use_all_bones=True,
            bake_anim_use_nla_strips=True,
            bake_anim_use_all_actions=True,
            bake_anim_force_startend_keying=True,
            bake_anim_step=1.0,
            bake_anim_simplify_factor=1.0,
            path_mode='AUTO',
            embed_textures=True,
            batch_mode='OFF',
            use_batch_own_dir=False,
            use_metadata=True,
            axis_forward='-Z',
            axis_up='Y')
        MessageBox("3D Model Exported, please reload O3DE Level", "O3DE Tools", "LIGHT")
        if not bpy.types.Scene.exportInTextureFolder is None:
            ReplaceStoredPaths()
    else:
        MessageBox("Nothing Selected!", "O3DE Tools", "ERROR")

def CloneAndRepathImages(fileMenuExport, textureFilePath):
    """
    This function will copy project texture files to a
    O3DE textures folder and repath the Blender materials
    then repath them to thier orginal.
    """
    # Lets create a dictionary to store all the source paths to place back after export
    projectSelectionList = BuildProjectsList()
    bpy.types.Scene.storedImageSourcePathsDict = {}

    # Make or do not make a texture folder
    if fileMenuExport:
        # FILE MENU MENU EXPORT
        if bpy.types.Scene.exportInTextureFolder:
            # We do want the texture folder
            textureFilePath = os.path.join(os.path.dirname(textureFilePath), 'textures' )
            if not os.path.exists(textureFilePath):
                os.makedirs(textureFilePath)
            LoopThroughSelectedMaterailsToExport(textureFilePath)

        else:
            # We do not want the texture folder
            textureFilePath = os.path.dirname(textureFilePath)
            LoopThroughSelectedMaterailsToExport(textureFilePath)

        # Check to see if this project is listed in our projectSelectionList if not add it.
        if bpy.types.Scene.selectedo3deProjectPath not in projectSelectionList:
            projectSelectionList.append((bpy.types.Scene.selectedo3deProjectPath, os.path.basename(bpy.types.Scene.selectedo3deProjectPath), bpy.types.Scene.selectedo3deProjectPath))
        bpy.types.Scene.projectsWorkingList = EnumProperty(items=projectSelectionList, name='')
    elif fileMenuExport is None:
        # TOOL MENU EXPORT BUT WAS EXPORTED ONCE IN FILE MENU
        if bpy.types.Scene.exportInTextureFolder:
            textureFilePath = os.path.join(bpy.types.Scene.selectedo3deProjectPath, 'textures' )
            LoopThroughSelectedMaterailsToExport(textureFilePath)
        else:
            textureFilePath = os.path.join(bpy.types.Scene.selectedo3deProjectPath)
            if not os.path.exists(textureFilePath):
                os.makedirs(textureFilePath)
            LoopThroughSelectedMaterailsToExport(textureFilePath)
    else:
        # TOOL MENU EXPORT
        if bpy.types.Scene.exportInTextureFolder:
            # We want the texture folder
            textureFilePath = os.path.join(bpy.types.Scene.selectedo3deProjectPath, 'Assets', 'textures' )
            LoopThroughSelectedMaterailsToExport(textureFilePath)
        else:
            # We do not the texture folder
            textureFilePath = os.path.join(bpy.types.Scene.selectedo3deProjectPath, 'Assets')
            LoopThroughSelectedMaterailsToExport(textureFilePath)

def CheckFilePathsDuplicate(sourcePath, destPath):
    """
    This function check to see if Source Path and Dest Path are the same
    """
    try:
        fileAlreadyExist = os.path.samefile(sourcePath, destPath)
        if fileAlreadyExist:
            return True
        else:
            return False
    except FileNotFoundError:
        pass

def LoopThroughSelectedMaterailsToExport(textureFilePath):
    """
    This function will loop the the selected materials and copy the files to the o3de project folder
    """
    if not os.path.exists(textureFilePath):
            os.makedirs(textureFilePath)
    # retrive the list of seleted mesh materals
    selectedMaterials = GetSelectedMaterials()
    # Loop through Materials
    for mat in selectedMaterials:
        # Get the material
        material = bpy.data.materials[mat]
        # Loop through material node tree and get all the texture iamges
        for img in material.node_tree.nodes:
            if img.type == 'TEX_IMAGE':
                fullPath = bpy.path.abspath(img.image.filepath, library=img.image.library)
                sourcePath = os.path.normpath(fullPath)
                baseName = os.path.basename(sourcePath)
                o3deTexturePath = os.path.join(textureFilePath, baseName)
                # Add to storedImageSourcePathsDict to replace later
                if not CheckFilePathsDuplicate(sourcePath, o3deTexturePath): # We check first if its not already copied over.
                    bpy.types.Scene.storedImageSourcePathsDict[img.image.name] = sourcePath
                    # Copy the image to Destination
                    try:
                        shutil.copyfile(sourcePath, o3deTexturePath)
                        # Find image and repath
                        bpy.data.images[img.image.name].filepath = o3deTexturePath
                        # Save image to location
                        bpy.data.images[img.image.name].save()
                    except FileNotFoundError:
                        pass
                img.image.reload()

def ReplaceStoredPaths():
    """
    This Function will replace all the repathed image paths to thier origninal source paths
    """
    for image in bpy.data.images:
        try:
            # Find image and replace source path
            bpy.data.images[image.name].filepath = bpy.types.Scene.storedImageSourcePathsDict[image.name]
        except KeyError:
            pass
        image.reload()

    bpy.types.Scene.storedImageSourcePathsDict = {}

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
        fbxFileExporter()
        return{'FINISHED'}

def BuildProjectsList():
    """
    This function will build the project list
    """
    o3deDefaultProjectsFolder, o3deProjects = LookatEngineManifest()

    listOfo3deProjects = []
    
    if bpy.types.Scene.engineIsInstalled:
        # Make a list of projects to select from
        for projectFullPath in o3deProjects:
            listOfo3deProjects.append((projectFullPath, os.path.basename(projectFullPath), projectFullPath))
    return listOfo3deProjects

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
        fbxFileExporter(self.filepath)
        return{'FINISHED'}

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

        if bpy.types.Scene.engineIsInstalled: # Checks to see if O3DE is installed

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
            ExportFiles = layout.row()
            if bpy.types.Scene.selectedo3deProjectPath == '':
                ExportFiles.enabled = False
            else:
                ExportFiles.enabled = True
            # This is the export button
            ExportFiles.operator('vin.o3defilexport', text='EXPORT TO O3DE', icon="BLENDER")
        else:
            # If O3DE is not installed we tell the user
            row.operator("vin.wiki", text='O3DE Tools Wiki', icon="WORLD_DATA")
            notInstalled = layout.row()
            notInstalled.label(text='O3DE Needs to be installed')

def FileExportMenuAdd(self, context):
    """
    This Function will add the Export to O3DE to the file menu Export
    """
    self.layout.operator(SceneExporterFileMenu.bl_idname, text="Export to O3DE")

def register():
    auto_load.register()
    bpy.utils.register_class(O3deTools)
    bpy.utils.register_class(WikiButton)
    bpy.utils.register_class(ExportFiles)
    bpy.utils.register_class(Projects_OT_ListDropDown)
    bpy.utils.register_class(ExportOptions_OT_ListDropDown)
    bpy.utils.register_class(SceneExporterFileMenu)
    bpy.types.TOPBAR_MT_file_export.append(FileExportMenuAdd)
    bpy.types.Scene.selectedo3deEngine = ''
    bpy.types.Scene.selectedo3deProjectPath = ''
    bpy.types.Scene.engineIsInstalled = None
    bpy.types.Scene.exportInTextureFolder = True
    bpy.types.Scene.storedImageSourcePathsDict = {}
    bpy.types.Scene.projectsWorkingList = EnumProperty(items=BuildProjectsList(), name='')
    bpy.types.Scene.exportOptionsList = EnumProperty(items=EXPORT_LIST_OPTIONS, name='', default='0')


def unregister():
    auto_load.unregister()
    bpy.utils.unregister_class(O3deTools)
    bpy.utils.unregister_class(WikiButton)
    bpy.utils.unregister_class(ExportFiles)
    bpy.utils.unregister_class(Projects_OT_ListDropDown)
    bpy.utils.unregister_class(ExportOptions_OT_ListDropDown)
    bpy.utils.unregister_class(SceneExporterFileMenu)
    bpy.types.TOPBAR_MT_file_export.remove(FileExportMenuAdd)
    del bpy.types.Scene.selectedo3deEngine
    del bpy.types.Scene.selectedo3deProjectPath
    del bpy.types.Scene.projectsWorkingList
    del bpy.types.Scene.exportOptionsList
    del bpy.types.Scene.engineIsInstalled
    del bpy.types.Scene.exportInTextureFolder
    del bpy.types.Scene.storedImageSourcePathsDict

if __name__ == "__main__":
    register()
