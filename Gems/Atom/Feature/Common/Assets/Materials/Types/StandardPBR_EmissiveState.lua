--------------------------------------------------------------------------------------
--
-- Copyright (c) Contributors to the Open 3D Engine Project.
-- For complete copyright and license terms please see the LICENSE at the root of this distribution.
--
-- SPDX-License-Identifier: Apache-2.0 OR MIT
--
--
--
----------------------------------------------------------------------------------------------------

function GetMaterialPropertyDependencies()
    return {"enable", "useTexture", "textureMap"}
end

function GetShaderOptionDependencies()
    return {"o_emissive_useTexture", "o_emissiveEnabled"}
end
 
function Process(context)
    local enable = context:GetMaterialPropertyValue_bool("enable")
    local textureMap = context:GetMaterialPropertyValue_Image("textureMap")
    local useTextureMap = context:GetMaterialPropertyValue_bool("useTexture")
    
    context:SetShaderOptionValue_bool("o_emissiveEnabled", enable)
    context:SetShaderOptionValue_bool("o_emissive_useTexture", enable and useTextureMap and textureMap ~= nil)
end

function ProcessEditor(context)
    local enable = context:GetMaterialPropertyValue_bool("enable")

    local mainVisibility
    if(enable) then
        mainVisibility = MaterialPropertyVisibility_Enabled
    else
        mainVisibility = MaterialPropertyVisibility_Hidden
    end
    
    context:SetMaterialPropertyVisibility("color", mainVisibility)
    context:SetMaterialPropertyVisibility("intensity", mainVisibility)
    context:SetMaterialPropertyVisibility("unit", mainVisibility)
    context:SetMaterialPropertyVisibility("textureMap", mainVisibility)
    context:SetMaterialPropertyVisibility("useTexture", mainVisibility)
    context:SetMaterialPropertyVisibility("textureMapUv", mainVisibility)

    if(enable) then
        local textureMap = context:GetMaterialPropertyValue_Image("textureMap")
        local useTextureMap = context:GetMaterialPropertyValue_bool("useTexture")

        if(textureMap == nil) then
            context:SetMaterialPropertyVisibility("useTexture", MaterialPropertyVisibility_Hidden)
            context:SetMaterialPropertyVisibility("textureMapUv", MaterialPropertyVisibility_Hidden)
        elseif(not useTextureMap) then
            context:SetMaterialPropertyVisibility("textureMapUv", MaterialPropertyVisibility_Disabled)
        end
    end
end
