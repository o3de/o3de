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
        "enableAnisotropy"
        , "factor"
        , "anisotropyAngle"
    }
end
 
function GetShaderOptionDependencies()
    return {"o_enableAnisotropy"}
end

function Process(context)
    local enableAnisotropy = context:GetMaterialPropertyValue_bool("enableAnisotropy")
end

function ProcessEditor(context)
    
    local enableAnisotropy = context:GetMaterialPropertyValue_bool("enableAnisotropy")
        
    local visibility
    if(enableAnisotropy) then
        visibility = MaterialPropertyVisibility_Enabled    
    else
        visibility = MaterialPropertyVisibility_Hidden
    end

    context:SetMaterialPropertyVisibility("factor", visibility)
    context:SetMaterialPropertyVisibility("anisotropyAngle", visibility)
end
