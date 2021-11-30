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
-- There are separate functors for 'handling the individual material layers' vs 'enabling the overall feature'.

function GetMaterialPropertyDependencies()
    return { 
        "layer1_clearCoat.enable",
        "layer2_clearCoat.enable",
        "layer3_clearCoat.enable" 
    }
end

function GetShaderOptionDependencies()
    return { "o_clearCoat_feature_enabled" }
end
 
function Process(context)
    local enable1 = context:GetMaterialPropertyValue_bool("layer1_clearCoat.enable")
    local enable2 = context:GetMaterialPropertyValue_bool("layer2_clearCoat.enable")
    local enable3 = context:GetMaterialPropertyValue_bool("layer3_clearCoat.enable")
    context:SetShaderOptionValue_bool("o_clearCoat_feature_enabled", enable1 or enable2 or enable3)
end
