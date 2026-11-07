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

-- Shader selection for the Material Canvas preview pipeline; every IncludeShader name needs a matching .shader.template entry.

function MaterialTypeSetup(context)
    lightingModel = context:GetLightingModelName()
    Print('Material Canvas preview pipeline: lighting model "' .. lightingModel .. '".')

    context:ExcludeAllShaders()

    -- No _CustomZ variants: ShaderEnable.lua falls back to the plain shaders, so only depth/shadow silhouettes lose the offset.

    opacityMode = context:GetBuildSetting("opacityMode", "Dynamic")
    if (opacityMode ~= "Dynamic" and opacityMode ~= "Opaque" and opacityMode ~= "Cutout" and
        opacityMode ~= "Blended" and opacityMode ~= "TintedTransparent") then
        Warning('Unrecognised "opacityMode" build setting "' .. opacityMode .. '". Building every shader.')
        opacityMode = "Dynamic"
    end

    buildForwardShader = opacityMode == "Dynamic" or opacityMode == "Opaque" or opacityMode == "Cutout"
    buildBlendedShader = opacityMode == "Dynamic" or opacityMode == "Blended"
    buildTintedTransparentShader = opacityMode == "Dynamic" or opacityMode == "TintedTransparent"
    buildVertexShaders = opacityMode == "Dynamic" or context:GetBuildSetting("positionOffset", "Disconnected") == "Connected"

    if (buildVertexShaders) then
        context:IncludeShader("DepthPass")
        context:IncludeShader("ShadowmapPass")
    end

    -- Base and Skin have no transparent shader in this pipeline, so a "Blended" declaration would leave nothing that draws.
    if (lightingModel == "Base") then
        if (buildBlendedShader or buildTintedTransparentShader) then
            Warning('The Base lighting model has no transparent shader. Building its forward shader instead.')
        end
        context:IncludeShader("ForwardPass_BaseLighting")
        return true
    end

    if (lightingModel == "Standard") then
        if (buildForwardShader) then
            context:IncludeShader("ForwardPass_StandardLighting")
        end
        if (buildBlendedShader) then
            context:IncludeShader("Transparent_StandardLighting")
        end
        if (buildTintedTransparentShader) then
            context:IncludeShader("TintedTransparent_StandardLighting")
        end
        return true
    end

    if (lightingModel == "Enhanced") then
        if (buildForwardShader) then
            context:IncludeShader("ForwardPass_EnhancedLighting")
        end
        if (buildBlendedShader) then
            context:IncludeShader("Transparent_EnhancedLighting")
        end
        if (buildTintedTransparentShader) then
            context:IncludeShader("TintedTransparent_EnhancedLighting")
        end
        return true
    end

    if (lightingModel == "Skin") then
        if (buildBlendedShader or buildTintedTransparentShader) then
            Warning('The Skin lighting model has no transparent shader. Building its forward shader instead.')
        end
        context:IncludeShader("ForwardPass_SkinLighting")
        return true
    end

    Error('Unsupported lighting model "' .. lightingModel .. '".')
    return false
end
