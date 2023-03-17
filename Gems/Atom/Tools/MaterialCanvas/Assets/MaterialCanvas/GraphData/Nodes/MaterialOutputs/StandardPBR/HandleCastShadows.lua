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

function GetMaterialPropertyDependencies()
    return {"general.cast_shadows"}
end

function Process(context)
    local castShadows = true
    if context:HasMaterialProperty("general.cast_shadows") then
        castShadows = context:GetMaterialPropertyValue_bool("general.cast_shadows")
    end

    local shadowMap = context:GetShaderByTag("Shadowmap")
    if shadowMap then
        shadowMap:SetEnabled(castShadows)
    end
end
