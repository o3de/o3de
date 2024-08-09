"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

from PySide2 import QtWidgets
import azlmbr.asset as asset
import azlmbr.atom
import azlmbr.bus
import azlmbr.math as math
import azlmbr.name
import azlmbr.paths
import azlmbr.shader
import azlmbr.shadermanagementconsole
import json
import os

# Make a copy of shaderVariants, update target option value and return copy, accumulate stableId
def updateOptionValue(shaderVariants, targetOptionName, targetValue, stableId):
    tempShaderVariants = []
    
    for variantInfo in shaderVariants:
        tempVariantInfo = azlmbr.shader.ShaderVariantInfo()
        options = {}

        for optionName in variantInfo.options:
            if targetOptionName == optionName:
                options[optionName] = targetValue
            else:
                options[optionName] = variantInfo.options[optionName]

        tempVariantInfo.options = options
        tempVariantInfo.stableId = stableId
        tempShaderVariants.append(tempVariantInfo)
        stableId += 1

    return tempShaderVariants, stableId

# alters the option group passed in argument by clearing internal options if predicate passes
def clearOptions(optionGroup, predicateTakingOptionName_clearIfYes):
    descriptors = optionGroup.GetShaderOptionDescriptors()
    for optDesc in descriptors:
        optionName = optDesc.GetName()
        if predicateTakingOptionName_clearIfYes(optionName):
            optionGroup.ClearValue(optionName)

# Input is one .shader
def create_shadervariantlist_for_shader(filename):

    print(f"Creating shader variant list for {filename}")

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
    print(f"Found {len(materialAssetIds)} material assets referencing shader")

    shaderVariantList = azlmbr.shader.ShaderVariantListSourceData()
    shaderVariantList.shaderFilePath = shaderAssetInfo.relativePath

    if len(materialAssetIds) == 0:
        # No material is using this shader, so we can't get ShaderOptionDescriptor in the script
        # Return early and handle user assigned system option after shaderAsset is loaded
        return shaderVariantList
    
    # This loop collects all uniquely-identified shader items used by the materials based on its shader variant id. 
    shader_file = os.path.basename(filename)
    shaderVariantIds = []
    shaderOptionGroups = []

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
                    optionGroup = shaderItem.GetShaderOptionGroup()
                    # clear the group from spuriously defaulted options by reducing it to a lean set:
                    clearOptions(optionGroup, lambda optName_Query: not shaderItem.MaterialOwnsShaderOption(optName_Query))
                    shaderOptionGroups.append(optionGroup)

        progressDialog.setValue(i)

        # processing events to update UI after progress bar changes
        QtWidgets.QApplication.processEvents()

        # Allowing the application to process idle events for one frame to update systems and garbage collect graphics resources
        azlmbr.atomtools.general.idle_wait_frames(1)
        if progressDialog.wasCanceled():
            return

    progressDialog.close()

    # Read from shaderPath.systemoptions to get user assigned system option value
    pre, ext = os.path.splitext(filename)
    systemOptionFilePath = f'{pre}.systemoptions'
    systemOptionFilePath = systemOptionFilePath.replace("\\", "/")
    systemOptionDict = {}

    if os.path.isfile(systemOptionFilePath):
        with open(systemOptionFilePath, "r") as systemOptionFile:
            systemOptionDict = json.load(systemOptionFile)

    # Generate the shader variant list data by collecting shader option name-value pairs.
    shaderVariants = []
    systemOptionDescriptor = {}
    stableId = 1

    for shaderOptionGroup in shaderOptionGroups:
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

            # Check user assigned value
            optionNameString = optionName.ToString()
            if optionNameString in systemOptionDict:
                if systemOptionDict[optionNameString] != "":
                    options[optionName] = azlmbr.name.Name(systemOptionDict[optionNameString])
                else:
                    # Value is unset, expansion handling later
                    systemOptionDescriptor[optionName] = shaderOptionDescriptor

        if len(options) != 0:
            variantInfo.options = options
            shaderVariants.append(variantInfo)
            stableId += 1

    shaderVariantList.materialOptionsHint = set(options.keys()) - set(systemOptionDict.keys())

    # Expand the unset system option
    for systemOptionName in systemOptionDescriptor:
        optionDescriptor = systemOptionDescriptor[systemOptionName]
        defaultValue = optionDescriptor.GetDefaultValue()
        
        valueMin = optionDescriptor.GetMinValue()
        valueMax = optionDescriptor.GetMaxValue()
        totalShaderVariants = []
        
        for index in range(valueMin.GetIndex(), valueMax.GetIndex() + 1):
            optionValue = optionDescriptor.GetValueNameByIndex(index)
            if optionValue != defaultValue:
                tempShaderVariants, stableId = updateOptionValue(shaderVariants, systemOptionName, optionValue, stableId) 
                totalShaderVariants.extend(tempShaderVariants)
            
        shaderVariants.extend(totalShaderVariants)

    shaderVariantList.shaderVariants = shaderVariants
    return shaderVariantList
