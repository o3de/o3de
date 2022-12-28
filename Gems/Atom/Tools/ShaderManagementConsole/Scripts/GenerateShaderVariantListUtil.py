"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import os
import azlmbr.asset as asset
import azlmbr.atom
import azlmbr.bus
import azlmbr.math as math
import azlmbr.name
import azlmbr.paths
import azlmbr.shadermanagementconsole
import azlmbr.shader
from PySide2 import QtWidgets

PROJECT_SHADER_VARIANTS_FOLDER = "ShaderVariants"

# input is Material Asset id list
def create_shadervariantlists_from_material(materialAssetIds):

    shaderVarintIdDict = {}     # Key: ShaderAssetId, Value: ShaderVarintId list
    shaderOptionGroupsDict = {} # Key: ShaderAssetId, Value: ShaderOptionGroups list

    progressDialog = QtWidgets.QProgressDialog("Gather shader variant information...", "Cancel", 0, len(materialAssetIds))
    progressDialog.setMaximumWidth(400)
    progressDialog.setMaximumHeight(100)
    progressDialog.setModal(True)
    progressDialog.setWindowTitle("Gather shader variants from material assets")
    
    # This loop collects all uniquely-identified shader items used by the materials based on its shader variant id. 
    for i, materialAssetId in enumerate(materialAssetIds):
        materialInstanceShaderItems = azlmbr.shadermanagementconsole.ShaderManagementConsoleRequestBus(azlmbr.bus.Broadcast, 'GetMaterialInstanceShaderItems', materialAssetId)

        for shaderItem in materialInstanceShaderItems:
            shaderAssetId = shaderItem.GetShaderAsset().get_id()
            shaderVariantId = shaderItem.GetShaderVariantId()

            if shaderVarintIdDict.get(shaderAssetId) == None:
                shaderVarintIdDict[shaderAssetId] = []
                shaderOptionGroupsDict[shaderAssetId] = []
                if not shaderVariantId.IsEmpty():
                    shaderVarintIdDict[shaderAssetId].append(shaderVariantId)
                    shaderOptionGroupsDict[shaderAssetId].append(shaderItem.GetShaderOptionGroup())
            else:
                if not shaderVariantId.IsEmpty():
                    # Check for repeat shader variant ids. We are using a list here
                    # instead of a set to check for duplicates on shaderVariantIds because
                    # shaderVariantId is not hashed by the ID like it is in the C++ side. 
                    has_repeat = False
                    for variantId in shaderVarintIdDict[shaderAssetId]:
                        if shaderVariantId == variantId:
                            has_repeat = True
                            break
                    if has_repeat:
                        continue

                    shaderVarintIdDict[shaderAssetId].append(shaderVariantId)
                    shaderOptionGroupsDict[shaderAssetId].append(shaderItem.GetShaderOptionGroup())

        progressDialog.setValue(i)
        if progressDialog.wasCanceled():
            return

    progressDialog.close()


    progressDialog = QtWidgets.QProgressDialog("Generating .shadervariantlist files...", "Cancel", 0, len(shaderOptionGroupsDict))
    progressDialog.setMaximumWidth(400)
    progressDialog.setMaximumHeight(100)
    progressDialog.setModal(True)
    progressDialog.setWindowTitle("Generating Shader Variant Lists")
    # Generate the shader variant list for each shader asset
    for i, shaderAssetId in enumerate(shaderOptionGroupsDict):

        shaderAssetInfo = asset.AssetCatalogRequestBus(
            azlmbr.bus.Broadcast, 
            'GetAssetInfoById', 
            shaderAssetId   
        )
        fullShaderPath = azlmbr.shadermanagementconsole.ShaderManagementConsoleRequestBus(
            azlmbr.bus.Broadcast, 
            'GetFullSourcePathFromRelativeProductPath', 
            shaderAssetInfo.relativePath
        )
        relativeShaderPath = azlmbr.shadermanagementconsole.ShaderManagementConsoleRequestBus(
            azlmbr.bus.Broadcast, 
            'GenerateRelativeSourcePath', 
            fullShaderPath
        )
        shaderVariantList = azlmbr.shader.ShaderVariantListSourceData()
        shaderVariantList.shaderFilePath = relativeShaderPath
        shaderVariants = []
        stableId = 1
        for shaderOptionGroup in shaderOptionGroupsDict[shaderAssetId]:
            variantInfo = azlmbr.shader.ShaderVariantInfo()
            variantInfo.stableId = stableId
            options = {}

            shaderOptionDescriptors = shaderOptionGroup.GetShaderOptionDescriptors()
            for shaderOptionDescriptor in shaderOptionDescriptors:
                optionName = shaderOptionDescriptor.GetName()
                optionValue = shaderOptionGroup.GetValueByOptionName(optionName)
                if not optionValue.IsValid():
                    continue

                valueName = shaderOptionDescriptor.GetValueName(optionValue)
                options[optionName] = valueName

            if len(options) != 0:
                variantInfo.options = options
                shaderVariants.append(variantInfo)
                stableId += 1

        if shaderVariants:
            shaderVariantList.shaderVariants = shaderVariants
            pre, ext = os.path.splitext(relativeShaderPath)
            if "Intermediate Assets" in fullShaderPath:
                projectShaderVariantListFilePath = os.path.join(azlmbr.paths.projectroot, PROJECT_SHADER_VARIANTS_FOLDER, "Cache", "Intermediate Assets", f'{pre}.shadervariantlist')
            else:
                projectShaderVariantListFilePath = os.path.join(azlmbr.paths.projectroot, PROJECT_SHADER_VARIANTS_FOLDER, f'{pre}.shadervariantlist')
            
            # clean previously generated shader variant list file so they don't clash.
            if os.path.exists(projectShaderVariantListFilePath):
                os.remove(projectShaderVariantListFilePath)
            
            azlmbr.shader.SaveShaderVariantListSourceData(projectShaderVariantListFilePath, shaderVariantList)
        
        progressDialog.setValue(i)
        if progressDialog.wasCanceled():
            return
    
    progressDialog.close()


# Input is .shader
def create_shadervariantlist_for_shader(filename):
    
    # Get info such as relative path of the file and asset id
    shaderAssetInfo = azlmbr.shadermanagementconsole.ShaderManagementConsoleRequestBus(
        azlmbr.bus.Broadcast, 
        'GetSourceAssetInfo', 
        filename
    )
    # retrieves a list of all material source files that use the shader. Note that materials inherit from materialtype files, which are actual files that refer to shader files.
    materialAssetIds = azlmbr.shadermanagementconsole.ShaderManagementConsoleRequestBus(
        azlmbr.bus.Broadcast, 
        'FindMaterialAssetsUsingShader', 
        shaderAssetInfo.relativePath
    )


    # This loop collects all uniquely-identified shader items used by the materials based on its shader variant id. 
    shader_file = os.path.basename(filename)
    shaderVariantIds = []
    shaderVariantListShaderOptionGroups = []
    progressDialog = QtWidgets.QProgressDialog(f"Generating .shadervariantlist file for:\n{shader_file}", "Cancel", 0, len(materialAssetIds))
    progressDialog.setMaximumWidth(400)
    progressDialog.setMaximumHeight(100)
    progressDialog.setModal(True)
    progressDialog.setWindowTitle("Generating Shader Variant List")
    for i, materialAssetId in enumerate(materialAssetIds):
        materialInstanceShaderItems = azlmbr.shadermanagementconsole.ShaderManagementConsoleRequestBus(azlmbr.bus.Broadcast, 'GetMaterialInstanceShaderItems', materialAssetId)

        for shaderItem in materialInstanceShaderItems:
            shaderAssetId = shaderItem.GetShaderAsset().get_id()
            if shaderAssetInfo.assetId == shaderAssetId:
                shaderVariantId = shaderItem.GetShaderVariantId()
                if not shaderVariantId.IsEmpty():
                    # Check for repeat shader variant ids. We are using a list here
                    # instead of a set to check for duplicates on shaderVariantIds because
                    # shaderVariantId is not hashed by the ID like it is in the C++ side. 
                    has_repeat = False
                    for variantId in shaderVariantIds:
                        if shaderVariantId == variantId:
                            has_repeat = True
                            break
                    if has_repeat:
                        continue

                    shaderVariantIds.append(shaderVariantId)
                    shaderVariantListShaderOptionGroups.append(shaderItem.GetShaderOptionGroup())

        progressDialog.setValue(i)
        if progressDialog.wasCanceled():
            return

    progressDialog.close()

    # Generate the shader variant list data by collecting shader option name-value pairs.s
    shaderVariantList = azlmbr.shader.ShaderVariantListSourceData()
    shaderVariantList.shaderFilePath = shaderAssetInfo.relativePath
    shaderVariants = []
    stableId = 1
    for shaderOptionGroup in shaderVariantListShaderOptionGroups:
        variantInfo = azlmbr.shader.ShaderVariantInfo()
        variantInfo.stableId = stableId
        options = {}

        shaderOptionDescriptors = shaderOptionGroup.GetShaderOptionDescriptors()
        for shaderOptionDescriptor in shaderOptionDescriptors:
            optionName = shaderOptionDescriptor.GetName()
            optionValue = shaderOptionGroup.GetValueByOptionName(optionName)
            if not optionValue.IsValid():
                continue

            valueName = shaderOptionDescriptor.GetValueName(optionValue)
            options[optionName] = valueName

        if len(options) != 0:
            variantInfo.options = options
            shaderVariants.append(variantInfo)
            stableId += 1

    shaderVariantList.shaderVariants = shaderVariants

    pre, ext = os.path.splitext(filename)
    defaultShaderVariantListFilePath = f'{pre}.shadervariantlist'
    defaultShaderVariantListFilePath = defaultShaderVariantListFilePath.replace("\\", "/")

    return shaderVariantList, defaultShaderVariantListFilePath

