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

    opacityMode = context:GetBuildSetting("opacityMode", "Dynamic")
    if(opacityMode ~= "Dynamic" and opacityMode ~= "Opaque" and opacityMode ~= "Cutout" and
       opacityMode ~= "Blended" and opacityMode ~= "TintedTransparent") then
        Warning('Unrecognised "opacityMode" build setting "' .. opacityMode .. '". Building every shader.')
        opacityMode = "Dynamic"
    end

    buildOpaqueShader = opacityMode == "Dynamic" or opacityMode == "Opaque"
    buildCutoutShaders = opacityMode == "Dynamic" or opacityMode == "Cutout"
    buildBlendedShaders = opacityMode == "Dynamic" or opacityMode == "Blended"
    buildTintedTransparentShaders = opacityMode == "Dynamic" or opacityMode == "TintedTransparent"

    -- The forward shader only lights what the depth prepass has already drawn, and only the shadowmap shader casts a shadow, so
    -- an opaque surface needs all three whether or not it moves its vertices. Base and Skin build nothing but their forward
    -- shader, so they always need them. Motion vectors follow the object's transform too, so every opacity mode builds them.
    buildDepthAndShadowShaders = buildOpaqueShader or (lightingModel ~= "Standard" and lightingModel ~= "Enhanced")

    if(buildDepthAndShadowShaders) then
        context:IncludeShader("DepthPass")
        context:IncludeShader("ShadowmapPass")
    end
    context:IncludeShader("MeshMotionVector")

    -- The Base and Skin lighting models have no transparent shader in this pipeline, so there is no opaque/transparent split to make.
    -- A "Blended" declaration on one of them would leave the material type with nothing that draws, so it is reported and ignored.
    if(lightingModel == "Base") then
        if(opacityMode == "Blended" or opacityMode == "TintedTransparent") then
            Warning('The Base lighting model has no transparent shader. Building its forward shader instead.')
        end
        context:IncludeShader("ForwardPass_BaseLighting")
        return true
    end

    if(lightingModel == "Standard") then
        if(buildOpaqueShader) then
            context:IncludeShader("ForwardPass_StandardLighting")
        end
        if(buildCutoutShaders) then
            context:IncludeShader("ForwardPass_StandardLighting_CustomZ")
            context:IncludeShader("DepthPass_CustomZ")
            context:IncludeShader("ShadowmapPass_CustomZ")
        end
        if(buildBlendedShaders) then
            context:IncludeShader("Transparent_StandardLighting")
        end
        if(buildTintedTransparentShaders) then
            context:IncludeShader("TintedTransparent_StandardLighting")
        end
        if(buildBlendedShaders or buildTintedTransparentShaders) then
            context:IncludeShader("DepthPassTransparentMin")
            context:IncludeShader("DepthPassTransparentMax")
        end
        return true
    end

    if(lightingModel == "Enhanced") then
        if(buildOpaqueShader) then
            context:IncludeShader("ForwardPass_EnhancedLighting")
        end
        if(buildCutoutShaders) then
            context:IncludeShader("ForwardPass_EnhancedLighting_CustomZ")
            context:IncludeShader("DepthPass_CustomZ")
            context:IncludeShader("ShadowmapPass_CustomZ")
        end
        if(buildBlendedShaders) then
            context:IncludeShader("Transparent_EnhancedLighting")
        end
        if(buildTintedTransparentShaders) then
            context:IncludeShader("TintedTransparent_EnhancedLighting")
        end
        if(buildBlendedShaders or buildTintedTransparentShaders) then
            context:IncludeShader("DepthPassTransparentMin")
            context:IncludeShader("DepthPassTransparentMax")
        end
        return true
    end

    if(lightingModel == "Skin") then
        if(opacityMode == "Blended" or opacityMode == "TintedTransparent") then
            Warning('The Skin lighting model has no transparent shader. Building its forward shader instead.')
        end
        context:IncludeShader("ForwardPass_SkinLighting")
        return true
    end

    Error('Unsupported lighting model "' .. lightingModel .. '".')
    return false
end
