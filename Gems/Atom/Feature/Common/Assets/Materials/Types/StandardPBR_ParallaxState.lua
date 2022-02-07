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
    return {"parallax.textureMap", "parallax.useTexture"}
end

function GetShaderOptionDependencies()
    return {"o_parallax_feature_enabled", "o_useHeightmap"}
end
 
function Process(context)
    local textureMap = context:GetMaterialPropertyValue_Image("parallax.textureMap")
    local useTexture = context:GetMaterialPropertyValue_bool("parallax.useTexture")
    local enable = textureMap ~= nil and useTexture 
    context:SetShaderOptionValue_bool("o_parallax_feature_enabled", enable)
    context:SetShaderOptionValue_bool("o_useHeightmap", enable)
end

function ProcessEditor(context)
    local textureMap = context:GetMaterialPropertyValue_Image("parallax.textureMap")
    
    if textureMap ~= nil then
        context:SetMaterialPropertyVisibility("parallax.useTexture", MaterialPropertyVisibility_Enabled)
    else 
        context:SetMaterialPropertyVisibility("parallax.useTexture", MaterialPropertyVisibility_Hidden)
    end
    
    local useTexture = context:GetMaterialPropertyValue_bool("parallax.useTexture")

    local visibility = MaterialPropertyVisibility_Enabled
    if(textureMap == nil) then
        visibility = MaterialPropertyVisibility_Hidden
    elseif not useTexture then
        visibility = MaterialPropertyVisibility_Disabled
    end
        
    context:SetMaterialPropertyVisibility("parallax.factor", visibility)
    context:SetMaterialPropertyVisibility("parallax.offset", visibility)
    context:SetMaterialPropertyVisibility("parallax.showClipping", visibility)
    context:SetMaterialPropertyVisibility("parallax.algorithm", visibility)
    context:SetMaterialPropertyVisibility("parallax.quality", visibility)
    context:SetMaterialPropertyVisibility("parallax.pdo", visibility)
    context:SetMaterialPropertyVisibility("parallax.textureMapUv", visibility)
end
