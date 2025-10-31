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
    return {
        "specularF0.enableMultiScatterCompensation"
    }
end
 
function GetShaderOptionDependencies()
    return {
        "o_specularF0_enableMultiScatterCompensation"
    }
end

function Process(context)
    local enableMultiScatterCompensation = context:GetMaterialPropertyValue_bool("specularF0.enableMultiScatterCompensation") 
end

function ProcessEditor(context)
    context:SetMaterialPropertyVisibility("specularF0.enableMultiScatterCompensation", MaterialPropertyVisibility_Hidden)
end