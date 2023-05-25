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

function MaterialTypeSetup(context)
    lightingModel = context:GetLightingModelName()
    Print('Material type uses lighting model "' .. lightingModel .. '".')

    context:ExcludeAllShaders()
    
    --TODO(MaterialPipeline): Add a way to configure this script through more than just lightingModel, for example
    --set a transparency built setting to "AlwaysOpaque", "AlwaysTransparent", or "Dynamic". That way some Material
    --types that don't care about transparency can set "AlwaysOpaque" and skip compiling the transparent shaders.
    
    context:IncludeShader("DepthPass")
    context:IncludeShader("ShadowmapPass")
    context:IncludeShader("MeshMotionVector")

    if(lightingModel == "Base") then
        context:IncludeShader("ForwardPass_BaseLighting")
        return true
    end

    if(lightingModel == "Standard") then
        context:IncludeShader("ForwardPass_StandardLighting")
        context:IncludeShader("ForwardPass_StandardLighting_CustomZ")
        context:IncludeShader("Transparent_StandardLighting")
        context:IncludeShader("TintedTransparent_StandardLighting")
        context:IncludeShader("DepthPass_CustomZ")
        context:IncludeShader("ShadowmapPass_CustomZ")
        context:IncludeShader("DepthPassTransparentMin")
        context:IncludeShader("DepthPassTransparentMax")
        return true
    end
    
    if(lightingModel == "Enhanced") then
        context:IncludeShader("ForwardPass_EnhancedLighting")
        context:IncludeShader("ForwardPass_EnhancedLighting_CustomZ")
        context:IncludeShader("Transparent_EnhancedLighting")
        context:IncludeShader("TintedTransparent_EnhancedLighting")
        context:IncludeShader("DepthPass_CustomZ")
        context:IncludeShader("ShadowmapPass_CustomZ")
        context:IncludeShader("DepthPassTransparentMin")
        context:IncludeShader("DepthPassTransparentMax")
        return true
    end
    
    if(lightingModel == "Skin") then
        context:IncludeShader("ForwardPass_SkinLighting")
        return true
    end
     
    Error('Unsupported lighting model "' .. lightingModel .. '".')
    return false
end

