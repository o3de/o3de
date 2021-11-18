--------------------------------------------------------------------------------------
--
-- All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
-- its licensors.
--
-- For complete copyright and license terms please see the LICENSE at the root of this
-- distribution (the "License"). All use of this software is governed by the License,
-- or, if provided, by the license below or the license accompanying this file. Do not
-- remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
-- WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
