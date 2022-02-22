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
        "enable", 
        "influenceMap", 
        "useInfluenceMap", 
        "roughnessMap", 
        "useRoughnessMap", 
        "normalMap", 
        "useNormalMap", 
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
    local enable = context:GetMaterialPropertyValue_bool("enable")
    context:SetShaderOptionValue_bool("o_clearCoat_enabled", enable)
    
    UpdateUseTextureState(context, enable, "influenceMap", "useInfluenceMap", "o_clearCoat_factor_useTexture")
    UpdateUseTextureState(context, enable, "roughnessMap", "useRoughnessMap", "o_clearCoat_roughness_useTexture")
    UpdateUseTextureState(context, enable, "normalMap", "useNormalMap", "o_clearCoat_normal_useTexture")
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

function UpdateNormalStrengthPropertyVisibility(context, textureMapPropertyName, useTexturePropertyName)
    local textureMap = context:GetMaterialPropertyValue_Image(textureMapPropertyName)
    local useTexture = context:GetMaterialPropertyValue_bool(useTexturePropertyName)

    if(textureMap == nil) or (not useTexture) then
        context:SetMaterialPropertyVisibility("normalStrength", MaterialPropertyVisibility_Hidden)
    end
end

function ProcessEditor(context)
    local enable = context:GetMaterialPropertyValue_bool("enable")
    
    local mainVisibility
    if(enable) then
        mainVisibility = MaterialPropertyVisibility_Enabled
    else
        mainVisibility = MaterialPropertyVisibility_Hidden
    end
    
    context:SetMaterialPropertyVisibility("factor", mainVisibility)
    context:SetMaterialPropertyVisibility("influenceMap", mainVisibility)
    context:SetMaterialPropertyVisibility("useInfluenceMap", mainVisibility)
    context:SetMaterialPropertyVisibility("influenceMapUv", mainVisibility)
    context:SetMaterialPropertyVisibility("roughness", mainVisibility)
    context:SetMaterialPropertyVisibility("roughnessMap", mainVisibility)
    context:SetMaterialPropertyVisibility("useRoughnessMap", mainVisibility)
    context:SetMaterialPropertyVisibility("roughnessMapUv", mainVisibility)
    context:SetMaterialPropertyVisibility("normalMap", mainVisibility)
    context:SetMaterialPropertyVisibility("useNormalMap", mainVisibility)
    context:SetMaterialPropertyVisibility("normalMapUv", mainVisibility)
    context:SetMaterialPropertyVisibility("normalStrength", mainVisibility)

    if(enable) then
        UpdateTextureDependentPropertyVisibility(context, "influenceMap", "useInfluenceMap", "influenceMapUv")
        UpdateTextureDependentPropertyVisibility(context, "roughnessMap", "useRoughnessMap", "roughnessMapUv")
        UpdateTextureDependentPropertyVisibility(context, "normalMap", "useNormalMap", "normalMapUv")
        UpdateNormalStrengthPropertyVisibility(context, "normalMap", "useNormalMap")
    end
end
