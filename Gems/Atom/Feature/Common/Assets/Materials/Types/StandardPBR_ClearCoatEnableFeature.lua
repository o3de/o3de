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

-- This functor controls the flag that enables the overall feature for the shader.
-- Note multi-layer versions this material type have separate functors for 'handling the individual material layers' vs 'enabling the overall feature', so we follow the same pattern here for consistency.

function GetMaterialPropertyDependencies()
    return { "enable" }
end

function GetShaderOptionDependencies()
    return { "o_clearCoat_feature_enabled", "o_materialUseForwardPassIBLSpecular" }
end
 
function Process(context)
    local enable = context:GetMaterialPropertyValue_bool("enable")
    context:SetShaderOptionValue_bool("o_clearCoat_feature_enabled", enable)
    context:SetShaderOptionValue_bool("o_materialUseForwardPassIBLSpecular", enable)
end
