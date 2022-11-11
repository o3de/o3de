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
        context:IncludeShader("ForwardPass_BaseLighting")
        return true
    end

    if(lightingModel == "Standard" or lightingModel == "Enhanced") then
        if(lightingModel == "Enhanced") then
            Warning("The multi view pipeline does not support the Enhanced lighting model. Will use Standard lighting as a fallback.")
        end

        context:IncludeShader("ForwardPass_StandardLighting")
        return true
    end
    
    Error('Unsupported lighting model "' .. lightingModel .. '".')
    return false
end

