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
    
    local depthPass = context:GetShaderByTag("DepthPass")
    local shadowMap = context:GetShaderByTag("Shadowmap")
    local forwardPassEDS = context:GetShaderByTag("ForwardPass_EDS")

    local depthPassWithPS = context:GetShaderByTag("DepthPass_WithPS")
    local shadowMapWithPS = context:GetShaderByTag("Shadowmap_WithPS")
    local forwardPass = context:GetShaderByTag("ForwardPass")

    -- Use TryGetShaderByTag because these shaders only exist in StandardPBR but this script is also used for EnhancedPBR
    local lowEndForwardEDS = TryGetShaderByTag(context, "LowEndForward_EDS")
    local lowEndForward = TryGetShaderByTag(context, "LowEndForward")
    
    if parallaxEnabled and parallaxPdoEnabled then
        depthPass:SetEnabled(false)
        shadowMap:SetEnabled(false)
        forwardPassEDS:SetEnabled(false)
        
        depthPassWithPS:SetEnabled(true)
        shadowMapWithPS:SetEnabled(true)
        forwardPass:SetEnabled(true)

        TrySetShaderEnabled(lowEndForwardEDS, false)
        TrySetShaderEnabled(lowEndForward, true)
    else
        depthPass:SetEnabled(opacityMode == OpacityMode_Opaque)
        shadowMap:SetEnabled(opacityMode == OpacityMode_Opaque)
        forwardPassEDS:SetEnabled((opacityMode == OpacityMode_Opaque) or (opacityMode == OpacityMode_Blended) or (opacityMode == OpacityMode_TintedTransparent))
        
        depthPassWithPS:SetEnabled(opacityMode == OpacityMode_Cutout)
        shadowMapWithPS:SetEnabled(opacityMode == OpacityMode_Cutout)
        forwardPass:SetEnabled(opacityMode == OpacityMode_Cutout)

        TrySetShaderEnabled(lowEndForwardEDS, (opacityMode == OpacityMode_Opaque) or (opacityMode == OpacityMode_Blended) or (opacityMode == OpacityMode_TintedTransparent))
        TrySetShaderEnabled(lowEndForward, opacityMode == OpacityMode_Cutout)
    end
    
    context:GetShaderByTag("DepthPassTransparentMin"):SetEnabled((opacityMode == OpacityMode_Blended) or (opacityMode == OpacityMode_TintedTransparent))
    context:GetShaderByTag("DepthPassTransparentMax"):SetEnabled((opacityMode == OpacityMode_Blended) or (opacityMode == OpacityMode_TintedTransparent))
end
