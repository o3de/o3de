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

function GetMaterialPropertyDependencies()
    return { "enable" }
end

function GetShaderOptionDependencies()
    return { "o_useVertexColor" }
end
 
function Process(context)
    local enable = context:GetMaterialPropertyValue_bool("enable")
    context:SetShaderOptionValue_bool("o_useVertexColor", enable)
end

function ProcessEditor(context)
    local enable = context:GetMaterialPropertyValue_bool("enable")
    local mainVisibility
    if(enable) then
        mainVisibility = MaterialPropertyVisibility_Enabled
    else
        mainVisibility = MaterialPropertyVisibility_Disabled
    end

    context:SetMaterialPropertyVisibility("factor", mainVisibility)
    context:SetMaterialPropertyVisibility("blendMode", mainVisibility)    
end