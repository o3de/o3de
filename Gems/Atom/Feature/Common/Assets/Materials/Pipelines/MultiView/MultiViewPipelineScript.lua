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

    -- This pipeline declares no depth or shadow shader, so there is nothing for a "positionOffset" build setting to
    -- gate here. The other pipelines read it to decide whether to build their vertex-only passes.
    
    -- The Base lighting model has no transparent shader in this pipeline, so there is no opaque/transparent split to
    -- make. A "Blended" declaration on it would leave the material type with nothing that draws, so it is reported and
    -- ignored.
    if(lightingModel == "Base") then
        if(buildBlendedShaders or buildTintedTransparentShaders) then
            Warning('The Base lighting model has no transparent shader. Building its forward shader instead.')
        end
        context:IncludeShader("ForwardPass_BaseLighting")
        return true
    end

    if(lightingModel == "Standard" or lightingModel == "Enhanced") then
        if(lightingModel == "Enhanced") then
            Warning("The multi view pipeline does not support the Enhanced lighting model. Will use Standard lighting as a fallback.")
        end
        
        if(buildOpaqueShader) then
            context:IncludeShader("ForwardPass_StandardLighting")
        end
        if(buildCutoutShaders) then
            context:IncludeShader("ForwardPass_StandardLighting_CustomZ")
        end
        if(buildBlendedShaders) then
            context:IncludeShader("Transparent_StandardLighting")
        end
        if(buildTintedTransparentShaders) then
            context:IncludeShader("TintedTransparent_StandardLighting")
        end
        return true
    end
    
    if(lightingModel == "Skin") then
        Warning("The MultiView pipeline does not support the Skin lighting model. This combination should not be used at runtime.")
        -- This returns 'true' to pass the build, the surface won't be rendered at runtime.
        -- TODO(MaterialPipeline): Instead of rendering nothing, either render an error shader (like a magenta surface) or fall back to StandardLighting.
        --                         For an error shader, .materialtype needs to have new field for an ObjectSrg azsli file separate from "materialShaderCode", so that
        --                         the error shader can use the same ObjectSrg as the other shaders (depth/shadow) without including the unsupported materialShaderCode.
        --                         Using StandardLighting as a fallback is even more difficult because it requires some kind of adapter to move data from the Surface that
        --                         the material type wants to use, to the Surface that the lighting model supports. (It's a natural fit for downgrading from Enhanced to
        --                         Standard but there is compatibility issues between Skin and Standard).
        return true
    end

    Error('Unsupported lighting model "' .. lightingModel .. '".')
    return false
end

