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
    return {"emissive.enable", "emissive.useTexture", "emissive.textureMap"}
end

function GetShaderOptionDependencies()
    return {"o_emissive_useTexture", "o_emissiveEnabled"}
end
 
function Process(context)
    local enable = context:GetMaterialPropertyValue_bool("emissive.enable")
    local textureMap = context:GetMaterialPropertyValue_image("emissive.textureMap")
    local useTextureMap = context:GetMaterialPropertyValue_bool("emissive.useTexture")
    
    context:SetShaderOptionValue_bool("o_emissiveEnabled", enable)
    context:SetShaderOptionValue_bool("o_emissive_useTexture", enable and useTextureMap and textureMap ~= nil)
end

function ProcessEditor(context)
    local enable = context:GetMaterialPropertyValue_bool("emissive.enable")

    local mainVisibility
    if(enable) then
        mainVisibility = MaterialPropertyVisibility_Enabled
    else
        mainVisibility = MaterialPropertyVisibility_Hidden
    end
    
    context:SetMaterialPropertyVisibility("emissive.color", mainVisibility)
    context:SetMaterialPropertyVisibility("emissive.intensity", mainVisibility)
    context:SetMaterialPropertyVisibility("emissive.unit", mainVisibility)
    context:SetMaterialPropertyVisibility("emissive.textureMap", mainVisibility)
    context:SetMaterialPropertyVisibility("emissive.useTexture", mainVisibility)
    context:SetMaterialPropertyVisibility("emissive.textureMapUv", mainVisibility)

    if(enable) then
        local textureMap = context:GetMaterialPropertyValue_image("emissive.textureMap")
        local useTextureMap = context:GetMaterialPropertyValue_bool("emissive.useTexture")

        if(textureMap == nil) then
            context:SetMaterialPropertyVisibility("emissive.useTexture", MaterialPropertyVisibility_Hidden)
            context:SetMaterialPropertyVisibility("emissive.textureMapUv", MaterialPropertyVisibility_Hidden)
        elseif(not useTextureMap) then
            context:SetMaterialPropertyVisibility("emissive.textureMapUv", MaterialPropertyVisibility_Disabled)
        end
    end
end
