"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import sys
import os
import azlmbr.asset as asset
import azlmbr.atom
import azlmbr.bus
import azlmbr.math as math
import azlmbr.name
import azlmbr.paths
import azlmbr.shadermanagementconsole
import azlmbr.shader
import collections
from PySide2 import QtWidgets

PROJECT_SHADER_VARIANTS_FOLDER = "ShaderVariants"

def clean_existing_shadervariantlist_files(filePaths):
    for file in filePaths:
        if os.path.exists(file):
            os.remove(file)

def main():
    print("==== Begin shader variant script ==========================================================")

    if len(sys.argv) != 2:
        print("The script requires a .shader file as input argument")
        return

    filename = sys.argv[1]
    suffix, extension = filename.split(".", 1)

    if extension != "shader":
        print("The input argument for the script is not a valid .shader file")
        return

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

    # clean previously generated shader variant list file so they don't clash.
    pre, ext = os.path.splitext(shaderAssetInfo.relativePath)
    projectShaderVariantListFilePath = os.path.join(azlmbr.paths.projectroot, PROJECT_SHADER_VARIANTS_FOLDER, f'{pre}.shadervariantlist')
    pre, ext = os.path.splitext(filename)
    defaultShaderVariantListFilePath = f'{pre}.shadervariantlist'

    clean_existing_shadervariantlist_files([
        projectShaderVariantListFilePath
    ])

    defaultShaderVariantListFilePath = defaultShaderVariantListFilePath.replace("\\", "/")

    # Create shader variant list document
    documentId = azlmbr.atomtools.AtomToolsDocumentSystemRequestBus(
       azlmbr.bus.Broadcast,
       'CreateDocumentFromTypeName',
       'Shader Variant List'
    )
    # Update shader variant list
    azlmbr.shadermanagementconsole.ShaderManagementConsoleDocumentRequestBus(
        azlmbr.bus.Event,
        'SetShaderVariantListSourceData',
        documentId,
        shaderVariantList
    )
    
    print("==== End shader variant script ============================================================")

if __name__ == "__main__":
    main()

