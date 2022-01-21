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
import os
import shutil
import ui

def CheckSelected():
    """
    This function will check to see if the user has selected a object
    """
    selections = [obj.name for obj in bpy.context.selected_objects]
    if selections == []:
        ui.MessageBox("Nothing Selected....", "O3DE Tools", "ERROR")
        return False, "None"
    else:
        return True, selections

def GetSelectedMaterials(selected):
    """
    This function will check the selected mesh and find material are connected
    then build a material list for selected.
    """
    materials = []

    for obj in selected:
        try:
            materials.append(obj.active_material.name)
        except AttributeError:
            pass
    return materials

def LoopThroughSelectedMaterailsToExport(textureFilePath):
    """
    This function will loop the the selected materials and copy the files to the o3de project folder
    """
    if not os.path.exists(textureFilePath):
            os.makedirs(textureFilePath)
    # retrive the list of seleted mesh materals
    selectedMaterials = GetSelectedMaterials(bpy.context.selected_objects)
    # Loop through Materials
    for mat in selectedMaterials:
        # Get the material
        material = bpy.data.materials[mat]
        # Loop through material node tree and get all the texture iamges
        for img in material.node_tree.nodes:
            if img.type == 'TEX_IMAGE':
                # Frist make sure the image is not packed inside blender
                if img.image.packed_file:
                    bpy.ops.image.unpack(id=img.image.name)
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

def CloneAndRepathImages(fileMenuExport, textureFilePath, projectSelectionList):
    """
    This function will copy project texture files to a
    O3DE textures folder and repath the Blender materials
    then repath them to thier orginal.
    """
    # Lets create a dictionary to store all the source paths to place back after export
    #projectSelectionList = o3de.BuildProjectsList()
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