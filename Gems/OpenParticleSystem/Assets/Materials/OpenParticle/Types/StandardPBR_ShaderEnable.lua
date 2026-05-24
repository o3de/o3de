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
    return { "opacity.mode" }
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

    local opacityMode = OpacityMode_Opaque
    if context:HasMaterialProperty("opacity.mode") then
        opacityMode = context:GetMaterialPropertyValue_enum("opacity.mode")
    end

    -- Check if wind animation is enabled, if so, only enable WithPS shaders
    local windAnimationEnabled = false
    if context:HasMaterialProperty("windAnimation.windAnimation") then
        windAnimationEnabled = context:GetMaterialPropertyValue_bool("windAnimation.windAnimation")
    end

    local depthPass = context:GetShaderByTag("DepthPass")
    local shadowMap = context:GetShaderByTag("Shadowmap")
    local forwardPassEDS = context:GetShaderByTag("ForwardPass_EDS")

    local forwardPass = context:GetShaderByTag("ForwardPass")

    -- Use TryGetShaderByTag because these shaders only exist in StandardPBR but this script is also used for
    -- EnhancedPBR
    local lowEndForwardEDS = TryGetShaderByTag(context, "LowEndForward_EDS")
    local lowEndForward = TryGetShaderByTag(context, "LowEndForward")

    depthPass:SetEnabled(opacityMode == OpacityMode_Opaque)
    shadowMap:SetEnabled(opacityMode == OpacityMode_Opaque)
    forwardPassEDS:SetEnabled((opacityMode == OpacityMode_Opaque) or (opacityMode == OpacityMode_Blended)
            or (opacityMode == OpacityMode_TintedTransparent))

    forwardPass:SetEnabled(opacityMode == OpacityMode_Cutout)

    -- Only enable lowEndForwardEDS in Opaque mode, Transparent mode will be handled by forwardPassEDS.
    -- The transparent pass uses the "transparent" draw tag for both standard and low end pipelines, so this
    -- keeps both shaders from rendering to the transparent draw list.
    TrySetShaderEnabled(lowEndForwardEDS, opacityMode == OpacityMode_Opaque)
    TrySetShaderEnabled(lowEndForward, opacityMode == OpacityMode_Cutout)

    if context:HasShaderWithTag("DepthPassTransparentMin") then
        context:GetShaderByTag("DepthPassTransparentMin"):SetEnabled((opacityMode == OpacityMode_Blended)
                or (opacityMode == OpacityMode_TintedTransparent))
    end
    if context:HasShaderWithTag("DepthPassTransparentMax") then
        context:GetShaderByTag("DepthPassTransparentMax"):SetEnabled((opacityMode == OpacityMode_Blended)
                or (opacityMode == OpacityMode_TintedTransparent))
    end
end
