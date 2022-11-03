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
    
    if(lightingModel == "Base") then
        context:IncludeShader("DepthPass")
        context:IncludeShader("ShadowmapPass")
        context:IncludeShader("ForwardPass_BaseLighting")
        return true
    end

    if(lightingModel == "Standard") then
        context:IncludeShader("DepthPass")
        context:IncludeShader("ShadowmapPass")
        context:IncludeShader("ForwardPass_StandardLighting")
        return true
    end
    
    if(lightingModel == "Enhanced") then
        context:IncludeShader("DepthPass")
        context:IncludeShader("ShadowmapPass")
        context:IncludeShader("ForwardPass_EnhancedLighting")
        return true
    end

    Error('Unsupported lighting model "' .. lightingModel .. '".')
    return false
end

