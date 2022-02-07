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
    return {
        "clearCoat.enable", 
        "clearCoat.influenceMap", 
        "clearCoat.useInfluenceMap", 
        "clearCoat.roughnessMap", 
        "clearCoat.useRoughnessMap", 
        "clearCoat.normalMap", 
        "clearCoat.useNormalMap", 
        }
end

function GetShaderOptionDependencies()
    return {
        "o_clearCoat_enabled",
        "o_clearCoat_factor_useTexture", 
        "o_clearCoat_roughness_useTexture", 
        "o_clearCoat_normal_useTexture"
        }
end
 
function UpdateUseTextureState(context, clearCoatEnabled, textureMapPropertyName, useTexturePropertyName, shaderOptionName) 
    local textureMap = context:GetMaterialPropertyValue_Image(textureMapPropertyName)
    local useTextureMap = context:GetMaterialPropertyValue_bool(useTexturePropertyName)
    context:SetShaderOptionValue_bool(shaderOptionName, clearCoatEnabled and useTextureMap and textureMap ~= nil)
end

function Process(context)
    local enable = context:GetMaterialPropertyValue_bool("clearCoat.enable")
    context:SetShaderOptionValue_bool("o_clearCoat_enabled", enable)
    
    UpdateUseTextureState(context, enable, "clearCoat.influenceMap", "clearCoat.useInfluenceMap", "o_clearCoat_factor_useTexture")
    UpdateUseTextureState(context, enable, "clearCoat.roughnessMap", "clearCoat.useRoughnessMap", "o_clearCoat_roughness_useTexture")
    UpdateUseTextureState(context, enable, "clearCoat.normalMap", "clearCoat.useNormalMap", "o_clearCoat_normal_useTexture")
end

-- Note this logic matches that of the UseTextureFunctor class.
function UpdateTextureDependentPropertyVisibility(context, textureMapPropertyName, useTexturePropertyName, uvPropertyName)
    local textureMap = context:GetMaterialPropertyValue_Image(textureMapPropertyName)
    local useTexture = context:GetMaterialPropertyValue_bool(useTexturePropertyName)

    if(textureMap == nil) then
        context:SetMaterialPropertyVisibility(useTexturePropertyName, MaterialPropertyVisibility_Hidden)
        context:SetMaterialPropertyVisibility(uvPropertyName, MaterialPropertyVisibility_Hidden)
    elseif(not useTexture) then
        context:SetMaterialPropertyVisibility(uvPropertyName, MaterialPropertyVisibility_Disabled)
    end
end

function ProcessEditor(context)
    local enable = context:GetMaterialPropertyValue_bool("clearCoat.enable")
    
    local mainVisibility
    if(enable) then
        mainVisibility = MaterialPropertyVisibility_Enabled
    else
        mainVisibility = MaterialPropertyVisibility_Hidden
    end
    
    context:SetMaterialPropertyVisibility("clearCoat.factor", mainVisibility)
    context:SetMaterialPropertyVisibility("clearCoat.influenceMap", mainVisibility)
    context:SetMaterialPropertyVisibility("clearCoat.useInfluenceMap", mainVisibility)
    context:SetMaterialPropertyVisibility("clearCoat.influenceMapUv", mainVisibility)
    context:SetMaterialPropertyVisibility("clearCoat.roughness", mainVisibility)
    context:SetMaterialPropertyVisibility("clearCoat.roughnessMap", mainVisibility)
    context:SetMaterialPropertyVisibility("clearCoat.useRoughnessMap", mainVisibility)
    context:SetMaterialPropertyVisibility("clearCoat.roughnessMapUv", mainVisibility)
    context:SetMaterialPropertyVisibility("clearCoat.normalMap", mainVisibility)
    context:SetMaterialPropertyVisibility("clearCoat.useNormalMap", mainVisibility)
    context:SetMaterialPropertyVisibility("clearCoat.normalMapUv", mainVisibility)

    if(enable) then
        UpdateTextureDependentPropertyVisibility(context, "clearCoat.influenceMap", "clearCoat.useInfluenceMap", "clearCoat.influenceMapUv")
        UpdateTextureDependentPropertyVisibility(context, "clearCoat.roughnessMap", "clearCoat.useRoughnessMap", "clearCoat.roughnessMapUv")
        UpdateTextureDependentPropertyVisibility(context, "clearCoat.normalMap", "clearCoat.useNormalMap", "clearCoat.normalMapUv")
    end
end
