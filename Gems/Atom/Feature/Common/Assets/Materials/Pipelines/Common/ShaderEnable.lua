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
function TrySetShaderEnabled(context, shaderTag, enabled)
    if(context:HasShaderWithTag(shaderTag)) then
        local shader = context:GetShaderByTag(shaderTag)
        if(shader) then
            --Print("Set shader enabled '" .. shaderTag .. "' = " .. tostring(enabled))
            shader:SetEnabled(enabled)
        end
    end
end

-- Enables a shader with @shaderTag, if that shader exists, and disables @fallbackShaderTag.
-- Otherwise enables the shader @fallbackShaderTag, if that shader exists.
function TrySetShaderEnabledWithFallback(context, shaderTag, fallbackShaderTag, enabled)
    if(context:HasShaderWithTag(shaderTag)) then
        local shader = context:GetShaderByTag(shaderTag)
        if(shader) then
            --Print("Set shader enabled '" .. shaderTag .. "' = " .. tostring(enabled))
            shader:SetEnabled(enabled)
            TrySetShaderEnabled(context, fallbackShaderTag, false)
        end
    else
        TrySetShaderEnabled(context, fallbackShaderTag, enabled)
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

    if hasPerPixelDepth or hasPerPixelClip then
        TrySetShaderEnabledWithFallback(context, "depth_customZ", "depth", enableDepthPass)
        TrySetShaderEnabledWithFallback(context, "shadow_customZ", "shadow", enableShadowPass)
        
        -- The main pass could have different names in different pipelines
        TrySetShaderEnabledWithFallback(context, "forward_customZ", "forward", enableMainPass)
        TrySetShaderEnabledWithFallback(context, "main_customZ", "main", enableMainPass)
    else
        TrySetShaderEnabled(context, "depth", enableDepthPass)
        TrySetShaderEnabled(context, "shadow", enableShadowPass)
        TrySetShaderEnabled(context, "depth_customZ", false)
        TrySetShaderEnabled(context, "shadow_customZ", false)
        
        -- The main pass could have different names in different pipelines
        TrySetShaderEnabled(context, "forward", enableMainPass)
        TrySetShaderEnabled(context, "main", enableMainPass)
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

end

