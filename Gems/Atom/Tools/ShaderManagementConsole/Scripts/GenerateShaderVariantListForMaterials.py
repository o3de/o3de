"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
    
    # TODO: [ATOM-14868] Add UI prompts in shader management console script once PySide2 is available
    
    # This loop collects all uniquely-identified shader items used by the materials based on its shader variant id. 
    shaderVariantIds = []
    shaderVariantListShaderOptionGroups = []
    for materialAssetId in materialAssetIds:
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
                    
    # Generate the shader variant list data by collecting shader option name-value pairs.
    _, shaderFileName = os.path.split(filename)
    shaderVariantList = azlmbr.shader.ShaderVariantListSourceData ()
    shaderVariantList.shaderFilePath = shaderFileName
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
            options[optionName.ToString()] = valueName.ToString()
            
        if len(options) != 0:
            variantInfo.options = options
            shaderVariants.append(variantInfo)
            stableId += 1
                
    shaderVariantList.shaderVariants = shaderVariants
    
    # Save the shader variant list file
    pre, ext = os.path.splitext(filename)
    shaderVariantListFilePath = f'{pre}.shadervariantlist'
    azlmbr.shader.SaveShaderVariantListSourceData(shaderVariantListFilePath, shaderVariantList)
    
    # Open the document in shader management console
    azlmbr.shadermanagementconsole.ShaderManagementConsoleDocumentSystemRequestBus(
        azlmbr.bus.Broadcast,
        'OpenDocument',
        shaderVariantListFilePath
    )
    
    print("==== End shader variant script ============================================================")

if __name__ == "__main__":
    main()

