--------------------------------------------------------------------------------------
--
-- Copyright (c) Contributors to the Open 3D Engine Project.
-- For complete copyright and license terms please see the LICENSE at the root of this distribution.
--
-- SPDX-License-Identifier: Apache-2.0 OR MIT
--
----------------------------------------------------------------------------------------------------

function GetMaterialPropertyDependencies()
    return {"castShadows"}
end

function Process(context)
    local castShadows = context:GetMaterialPropertyValue_bool("castShadows")

    if context:HasShaderWithTag("shadow") then
        local shadowShader = context:GetShaderByTag("shadow")
        if shadowShader then
            shadowShader:SetEnabled(castShadows)
        end
    end
end