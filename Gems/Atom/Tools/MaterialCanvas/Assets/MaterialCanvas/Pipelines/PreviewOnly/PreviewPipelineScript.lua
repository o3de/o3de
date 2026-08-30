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

-- Shader selection for the Material Canvas preview-only material pipeline.
--
-- This is the trimmed counterpart to MainPipelineScript.lua. It selects the smallest set of shaders that still renders a material
-- correctly in the Material Canvas viewport, so that editing a graph rebuilds four shaders instead of eleven (and none at all from
-- LowEndPipeline, which the accompanying registry stub removes).
--
-- Every name passed to IncludeShader is the stem of a .shader.template declared in MainPipeline.materialpipeline next to this file --
-- MaterialPipelineScriptRunner strips the folder, the ".template" extension and the ".shader" extension to build its lookup table, and
-- raises a script error for any name that is not in it. Do not add an IncludeShader call here without adding the matching template
-- there.

function MaterialTypeSetup(context)
    lightingModel = context:GetLightingModelName()
    Print('Material Canvas preview pipeline: lighting model "' .. lightingModel .. '".')

    context:ExcludeAllShaders()

    -- No _CustomZ variants anywhere below. ShaderEnable.lua reaches for them through TrySetShaderEnabledWithFallback, which falls back
    -- to the plain depth, shadow and forward shaders when they are absent, so per-pixel-depth and alpha-cutout materials still draw --
    -- their depth and shadow silhouettes are just the un-offset geometry.

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
