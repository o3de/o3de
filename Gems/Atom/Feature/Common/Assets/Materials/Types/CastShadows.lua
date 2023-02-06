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
    return {"general.castShadows"}
end

function Process(context)
    -- The only reason we need this script is when the material type uses GeneralCommonPropertyGroup.json
    -- which does not connect general.castShadows, and the only way to add a connection after importing
    -- a common .json file is to use a lua functor. If we had another way to add a connection to a property
    -- after defining it via $import, that would probably be simpler.
    local castShadows = context:GetMaterialPropertyValue_bool("general.castShadows")
    local shadowMap = context:GetShaderByTag("Shadowmap")
    shadowMap:SetEnabled(castShadows)
end
