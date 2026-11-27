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

-- This script can enable/disable shaders for commonly used material pipeline properties and shaders.
-- It attempts to be flexible, using "Try" functions, that avoid strict requirements on particular 
-- properties or shader tags. So this script should cover many cases but any material pipeline can
-- of course provide its own script for enabling shaders, if needed.

function GetMaterialPropertyDependencies()
    return {"isTransparent", "isTintedTransparent", "castShadows", "hasPerPixelDepth", "hasPerPixelClip"}
end

-- Enables a shader with @shaderTag, if that shader exists.
-- Returns whether that shader was there to set.
function TrySetShaderEnabled(context, shaderTag, enabled)
    if(context:HasShaderWithTag(shaderTag)) then
        local shader = context:GetShaderByTag(shaderTag)
        if(shader) then
            --Print("Set shader enabled '" .. shaderTag .. "' = " .. tostring(enabled))
            shader:SetEnabled(enabled)
            return true
        end
    end
    return false
end

-- Enables a shader with @shaderTag, if that shader exists, and disables @fallbackShaderTag.
-- Otherwise enables the shader @fallbackShaderTag, if that shader exists.
-- Returns whether either of them was there to set.
function TrySetShaderEnabledWithFallback(context, shaderTag, fallbackShaderTag, enabled)
    if(context:HasShaderWithTag(shaderTag)) then
        local shader = context:GetShaderByTag(shaderTag)
        if(shader) then
            --Print("Set shader enabled '" .. shaderTag .. "' = " .. tostring(enabled))
            shader:SetEnabled(enabled)
            TrySetShaderEnabled(context, fallbackShaderTag, false)
            return true
        end
        return false
    else
        return TrySetShaderEnabled(context, fallbackShaderTag, enabled)
    end
end

function TryGetBoolProperty(context, propertyName, defaultValue) 
    if(context:HasMaterialProperty(propertyName)) then
      return context:GetMaterialPropertyValue_bool(propertyName)
    else
        return defaultValue
    end
end

function Process(context)
    
    --Print("========= ShaderEnable ==========")

    isTransparent = TryGetBoolProperty(context, "isTransparent", false)
    isTintedTransparent = TryGetBoolProperty(context, "isTintedTransparent", false)
    castShadows = TryGetBoolProperty(context, "castShadows", true)
    hasPerPixelDepth = TryGetBoolProperty(context, "hasPerPixelDepth", false)
    hasPerPixelClip = TryGetBoolProperty(context, "hasPerPixelClip", false)
    
    enableDepthPass = not isTransparent and not isTintedTransparent
    enableMainPass = not isTransparent and not isTintedTransparent
    enableShadowPass = not isTransparent and not isTintedTransparent and castShadows

    local mainPassResolved = false
    local depthPassResolved = false
    local shadowPassResolved = false

    if hasPerPixelDepth or hasPerPixelClip then
        depthPassResolved = TrySetShaderEnabledWithFallback(context, "depth_customZ", "depth", enableDepthPass)
        shadowPassResolved = TrySetShaderEnabledWithFallback(context, "shadow_customZ", "shadow", enableShadowPass)
        
        -- The main pass could have different names in different pipelines
        -- Assigned first and combined after: or between the two calls would skip the second.
        local resolvedForwardCustomZ = TrySetShaderEnabledWithFallback(context, "forward_customZ", "forward", enableMainPass)
        local resolvedMainCustomZ = TrySetShaderEnabledWithFallback(context, "main_customZ", "main", enableMainPass)
        mainPassResolved = resolvedForwardCustomZ or resolvedMainCustomZ
    else
        depthPassResolved = TrySetShaderEnabled(context, "depth", enableDepthPass)
        shadowPassResolved = TrySetShaderEnabled(context, "shadow", enableShadowPass)
        TrySetShaderEnabled(context, "depth_customZ", false)
        TrySetShaderEnabled(context, "shadow_customZ", false)
        
        -- The main pass could have different names in different pipelines
        local resolvedForward = TrySetShaderEnabled(context, "forward", enableMainPass)
        local resolvedMain = TrySetShaderEnabled(context, "main", enableMainPass)
        mainPassResolved = resolvedForward or resolvedMain
        TrySetShaderEnabled(context, "forward_customZ", false)
        TrySetShaderEnabled(context, "main_customZ", false)
    end

    
    if isTransparent and isTintedTransparent then
        Error("Material configuration conflict: isTransparent and isTintedTransparent are both true")
        TrySetShaderEnabled(context, "transparent", false)
        TrySetShaderEnabled(context, "tintedTransparent", false)
        TrySetShaderEnabled(context, "depthPassTransparentMin", false)
        TrySetShaderEnabled(context, "depthPassTransparentMax", false)
    else
        TrySetShaderEnabled(context, "transparent", isTransparent)
        TrySetShaderEnabled(context, "tintedTransparent", isTintedTransparent)
        TrySetShaderEnabled(context, "depthPassTransparentMin", isTransparent or isTintedTransparent)
        TrySetShaderEnabled(context, "depthPassTransparentMax", isTransparent or isTintedTransparent)
    end

    -- The Try functions stay silent about absent tags, which is normal across pipelines, but a shader this material needs
    -- may have been left out by the "opacityMode" build setting, and then nothing draws.
    if(isTransparent and not context:HasShaderWithTag("transparent")) then
        Warning('Material sets "isTransparent", but no transparent shader was built for this material pipeline. Nothing will be drawn.')
    end

    if(isTintedTransparent and not context:HasShaderWithTag("tintedTransparent")) then
        Warning('Material sets "isTintedTransparent", but no tinted transparent shader was built for this material pipeline. Nothing will be drawn.')
    end

    if(enableMainPass and not mainPassResolved) then
        Warning('Material is opaque, but no forward or main shader was built for this material pipeline. Nothing will be drawn.')
    end

    -- Not every pipeline has a depth prepass or a shadow pass, so a missing one is only reported when this pipeline built
    -- that pass for some other opacity mode. Without it the material is missing from the depth buffer, or casts no shadow.
    local pipelineHasDepthPass = context:HasShaderWithTag("depth") or context:HasShaderWithTag("depth_customZ")
    if(enableDepthPass and pipelineHasDepthPass and not depthPassResolved) then
        Warning('Material is opaque, but no depth shader for its opacity was built for this material pipeline. It will be missing from the depth prepass.')
    end

    local pipelineHasShadowPass = context:HasShaderWithTag("shadow") or context:HasShaderWithTag("shadow_customZ")
    if(enableShadowPass and pipelineHasShadowPass and not shadowPassResolved) then
        Warning('Material casts shadows, but no shadow shader for its opacity was built for this material pipeline. It will cast no shadow.')
    end

end

