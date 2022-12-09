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
    return {"isTransparent", "isTintedTransparent", "castShadows"}
end

function TrySetShaderEnabled(context, shaderTag, enabled)
    if(context:HasShaderWithTag(shaderTag)) then
        local shader = context:GetShaderByTag(shaderTag)
        if(shader) then
            shader:SetEnabled(enabled)
        end
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
    
    isTransparent = TryGetBoolProperty(context, "isTransparent", false)
    isTintedTransparent = TryGetBoolProperty(context, "isTintedTransparent", false)
    castShadows = TryGetBoolProperty(context, "castShadows", true)
    
    TrySetShaderEnabled(context, "depth", not isTransparent and not isTintedTransparent)
    TrySetShaderEnabled(context, "shadow", not isTransparent and not isTintedTransparent and castShadows)
    TrySetShaderEnabled(context, "forward", not isTransparent and not isTintedTransparent)
    
    if isTransparent and isTintedTransparent then
        Error("Material configuration conflict: isTransparent and isTintedTransparent are both true")
        TrySetShaderEnabled(context, "transparent", false)
        TrySetShaderEnabled(context, "tintedTransparent", false)
    else
        TrySetShaderEnabled(context, "transparent", isTransparent)
        TrySetShaderEnabled(context, "tintedTransparent", isTintedTransparent)
    end

end

