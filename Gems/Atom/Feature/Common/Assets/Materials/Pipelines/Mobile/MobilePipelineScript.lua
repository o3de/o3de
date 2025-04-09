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
    
    context:IncludeShader("ShadowmapPass")

    if(lightingModel == "Base") then
        context:IncludeShader("ForwardPass_BaseLighting")
        return true
    end

    if(lightingModel == "Standard" or lightingModel == "Enhanced") then
        if(lightingModel == "Enhanced") then
            Warning("The mobile pipeline does not support the Enhanced lighting model. Will use Standard lighting as a fallback.")
        end
        
        context:IncludeShader("ShadowmapPass_CustomZ")
        context:IncludeShader("ForwardPass_StandardLighting")
        context:IncludeShader("ForwardPass_StandardLighting_CustomZ")
        context:IncludeShader("Transparent_StandardLighting")
        context:IncludeShader("TintedTransparent_StandardLighting")
        return true
    end
    
    if(lightingModel == "Skin") then
        Error("The mobile pipeline does not support the Skin lighting model. This combination should not be used at runtime.")
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

