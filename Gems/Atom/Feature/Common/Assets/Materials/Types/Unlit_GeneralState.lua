--------------------------------------------------------------------------------------
--
-- Copyright (c) Contributors to the Open 3D Engine Project.
-- For complete copyright and license terms please see the LICENSE at the root of this distribution.
--
-- SPDX-License-Identifier: Apache-2.0 OR MIT
--
----------------------------------------------------------------------------------------------------

function GetMaterialPropertyDependencies()
    return {"general.castShadows", "opacity.mode"}
end

local OpacityMode_Blended = 2

function Process(context)
    local castShadows = context:GetMaterialPropertyValue_bool("general.castShadows")
    local opacityMode = context:GetMaterialPropertyValue_enum("opacity.mode")
    local enableShadowPass = castShadows and opacityMode ~= OpacityMode_Blended

    if context:HasShaderWithTag("shadow") then
        local shadowShader = context:GetShaderByTag("shadow")
        if shadowShader then
            shadowShader:SetEnabled(enableShadowPass)
        end
    end
end