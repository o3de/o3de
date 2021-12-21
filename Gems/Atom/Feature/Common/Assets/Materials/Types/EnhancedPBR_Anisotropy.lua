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
        "anisotropy.enableAnisotropy"
        , "anisotropy.factor"
        , "anisotropy.anisotropyAngle"
    }
end
 
function GetShaderOptionDependencies()
    return {"o_enableAnisotropy"}
end

function Process(context)
    local enableAnisotropy = context:GetMaterialPropertyValue_bool("anisotropy.enableAnisotropy")
end

function ProcessEditor(context)
    
    local enableAnisotropy = context:GetMaterialPropertyValue_bool("anisotropy.enableAnisotropy")
        
    local visibility
    if(enableAnisotropy) then
        visibility = MaterialPropertyVisibility_Enabled    
    else
        visibility = MaterialPropertyVisibility_Hidden
    end

    context:SetMaterialPropertyVisibility("anisotropy.factor", visibility)
    context:SetMaterialPropertyVisibility("anisotropy.anisotropyAngle", visibility)
end
