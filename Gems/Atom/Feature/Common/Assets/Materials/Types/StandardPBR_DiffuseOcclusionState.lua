--------------------------------------------------------------------------------------
--
-- All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
-- its licensors.
--
-- For complete copyright and license terms please see the LICENSE at the root of this
-- distribution (the "License"). All use of this software is governed by the License,
-- or, if provided, by the license below or the license accompanying this file. Do not
-- remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
-- WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
--
--
----------------------------------------------------------------------------------------------------

function GetMaterialPropertyDependencies()
    return {"occlusion.enable", "occlusion.diffuseTextureMap", "occlusion.diffuseUseTexture"}
end

function GetShaderOptionDependencies()
    return {"o_diffuseOcclusion_useTexture"}
end
 
function Process(context)
    local enableOcclusionMaps = context:GetMaterialPropertyValue_bool("occlusion.enable")
    local textureMap = context:GetMaterialPropertyValue_image("occlusion.diffuseTextureMap")
    local enableDiffuse = context:GetMaterialPropertyValue_bool("occlusion.diffuseUseTexture")

    context:SetShaderOptionValue_bool("o_diffuseOcclusion_useTexture", enableOcclusionMaps and textureMap ~= nil and enableDiffuse)
end

function ProcessEditor(context)
    local enableOcclusionMaps = context:GetMaterialPropertyValue_bool("occlusion.enable")

    if(not enableOcclusionMaps) then
        context:SetMaterialPropertyVisibility("occlusion.diffuseTextureMap", MaterialPropertyVisibility_Hidden)
        context:SetMaterialPropertyVisibility("occlusion.diffuseUseTexture", MaterialPropertyVisibility_Hidden)
        context:SetMaterialPropertyVisibility("occlusion.diffuseTextureMapUv", MaterialPropertyVisibility_Hidden)
        context:SetMaterialPropertyVisibility("occlusion.diffuseFactor", MaterialPropertyVisibility_Hidden)
    else
        context:SetMaterialPropertyVisibility("occlusion.diffuseTextureMap", MaterialPropertyVisibility_Enabled)
        local textureMap = context:GetMaterialPropertyValue_image("occlusion.diffuseTextureMap")
        if(textureMap == nil) then
            context:SetMaterialPropertyVisibility("occlusion.diffuseUseTexture", MaterialPropertyVisibility_Hidden)
            context:SetMaterialPropertyVisibility("occlusion.diffuseTextureMapUv", MaterialPropertyVisibility_Hidden)
            context:SetMaterialPropertyVisibility("occlusion.diffuseFactor", MaterialPropertyVisibility_Hidden)
        else
            context:SetMaterialPropertyVisibility("occlusion.diffuseUseTexture", MaterialPropertyVisibility_Enabled)
            context:SetMaterialPropertyVisibility("occlusion.diffuseTextureMapUv", MaterialPropertyVisibility_Enabled)
            context:SetMaterialPropertyVisibility("occlusion.diffuseFactor", MaterialPropertyVisibility_Enabled)
        end
    end
end
