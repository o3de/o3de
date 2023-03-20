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
    return {"textureMap", "useTexture", "pdo"}
end

function GetShaderOptionDependencies()
    return {"o_parallax_feature_enabled", "o_useHeightmap"}
end
 
function Process(context)
    local textureMap = context:GetMaterialPropertyValue_Image("textureMap")
    local useTexture = context:GetMaterialPropertyValue_bool("useTexture")
    local pixelDepthOffsetEnabled = context:GetMaterialPropertyValue_bool("pdo")

    local enable = textureMap ~= nil and useTexture 
    context:SetShaderOptionValue_bool("o_parallax_feature_enabled", enable)
    context:SetShaderOptionValue_bool("o_useHeightmap", enable)
    context:SetInternalMaterialPropertyValue_bool("hasPerPixelDepth", enable and pixelDepthOffsetEnabled)
end

function ProcessEditor(context)
    local textureMap = context:GetMaterialPropertyValue_Image("textureMap")
    
    if textureMap ~= nil then
        context:SetMaterialPropertyVisibility("useTexture", MaterialPropertyVisibility_Enabled)
    else 
        context:SetMaterialPropertyVisibility("useTexture", MaterialPropertyVisibility_Hidden)
    end
    
    local useTexture = context:GetMaterialPropertyValue_bool("useTexture")

    local visibility = MaterialPropertyVisibility_Enabled
    if(textureMap == nil) then
        visibility = MaterialPropertyVisibility_Hidden
    elseif not useTexture then
        visibility = MaterialPropertyVisibility_Disabled
    end
        
    context:SetMaterialPropertyVisibility("factor", visibility)
    context:SetMaterialPropertyVisibility("offset", visibility)
    context:SetMaterialPropertyVisibility("showClipping", visibility)
    context:SetMaterialPropertyVisibility("algorithm", visibility)
    context:SetMaterialPropertyVisibility("quality", visibility)
    context:SetMaterialPropertyVisibility("pdo", visibility)
    context:SetMaterialPropertyVisibility("shadowFactor", visibility)
    context:SetMaterialPropertyVisibility("textureMapUv", visibility)

    local pdoEnabled = context:GetMaterialPropertyValue_bool("pdo")
    if(not pdoEnabled) then
        context:SetMaterialPropertyVisibility("shadowFactor", MaterialPropertyVisibility_Hidden)
    end
end
