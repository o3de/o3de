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
    return {"opacity.mode", "parallax.textureMap", "parallax.useTexture", "parallax.pdo"}
end

OpacityMode_Opaque = 0
OpacityMode_Cutout = 1
OpacityMode_Blended = 2
OpacityMode_TintedTransparent = 3

function TryGetShaderByTag(context, shaderTag)
    if context:HasShaderWithTag(shaderTag) then
        return context:GetShaderByTag(shaderTag)
    else
        return nil
    end
end

function TrySetShaderEnabled(shader, enabled)
    if shader then 
        shader:SetEnabled(enabled)
    end
end

function Process(context)
    local opacityMode = context:GetMaterialPropertyValue_enum("opacity.mode")
    local displacementMap = context:GetMaterialPropertyValue_Image("parallax.textureMap")
    local useDisplacementMap = context:GetMaterialPropertyValue_bool("parallax.useTexture")
    local parallaxEnabled = displacementMap ~= nil and useDisplacementMap
    local parallaxPdoEnabled = context:GetMaterialPropertyValue_bool("parallax.pdo")
    
    local forwardPassEDS = context:GetShaderByTag("ForwardPass_EDS")
    
    forwardPassEDS:SetEnabled(true)
    
    context:GetShaderByTag("DepthPassTransparentMin_WithPS"):SetEnabled(true)
    context:GetShaderByTag("DepthPassTransparentMax_WithPS"):SetEnabled(true)
end
