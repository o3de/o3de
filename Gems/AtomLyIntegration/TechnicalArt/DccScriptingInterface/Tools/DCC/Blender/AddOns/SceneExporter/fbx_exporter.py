import bpy
import os
import re
import ui
import utils
import o3de
from pathlib import Path
# Optional Arguments for file menu exporter
#fbxPath = Path('')

def fbxFileExporter(fbxfilepath):
    """
    This function will send to selected .FBX to an O3DE Project Path
    """
    exportFilePath = ''
    # Validate a selection
    validSelection, selectedName = utils.CheckSelected()
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
                    utils.CloneAndRepathImages(fileMenuExport, bpy.types.Scene.selectedo3deProjectPath, projectSelectionList = o3de.BuildProjectsList())
            else:
                # WAS ONCE FILE MENU EXPORT
                exportFilePath = os.path.join(bpy.types.Scene.selectedo3deProjectPath, '{}{}'.format(filename, '.fbx'))
                # Clone Texture images and Repath images before export
                fileMenuExport = None # This is because it was first exported by the file menu export
                if not bpy.types.Scene.exportInTextureFolder == None:
                    utils.CloneAndRepathImages(fileMenuExport, bpy.types.Scene.selectedo3deProjectPath, projectSelectionList = o3de.BuildProjectsList())
        else:
            # Build new path
            exportFilePath = fbxfilepath
            bpy.types.Scene.selectedo3deProjectPath = os.path.dirname(fbxfilepath)
            # Clone Texture images and Repath images before export
            fileMenuExport = True
            if not bpy.types.Scene.exportInTextureFolder is None:
                utils.CloneAndRepathImages(fileMenuExport, fbxfilepath, projectSelectionList = o3de.BuildProjectsList())

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
        ui.MessageBox("3D Model Exported, please reload O3DE Level", "O3DE Tools", "LIGHT")
        if not bpy.types.Scene.exportInTextureFolder is None:
            utils.ReplaceStoredPaths()
    else:
        ui.MessageBox("Nothing Selected!", "O3DE Tools", "ERROR")