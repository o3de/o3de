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
    return {"roughness.textureMap", "roughness.useTexture"}
end
 
function GetShaderOptionDependencies()
    return {"o_roughness_useTexture"}
end

function Process(context)
    local textureMap = context:GetMaterialPropertyValue_Image("roughness.textureMap")
    local useTexture = context:GetMaterialPropertyValue_bool("roughness.useTexture")
    context:SetShaderOptionValue_bool("o_roughness_useTexture", useTexture and textureMap ~= nil)
end

function ProcessEditor(context)
    local textureMap = context:GetMaterialPropertyValue_Image("roughness.textureMap")
    local useTexture = context:GetMaterialPropertyValue_bool("roughness.useTexture")

    if(nil == textureMap) then
        context:SetMaterialPropertyVisibility("roughness.useTexture", MaterialPropertyVisibility_Hidden)
        context:SetMaterialPropertyVisibility("roughness.textureMapUv", MaterialPropertyVisibility_Hidden)

        context:SetMaterialPropertyVisibility("roughness.lowerBound", MaterialPropertyVisibility_Hidden)
        context:SetMaterialPropertyVisibility("roughness.upperBound", MaterialPropertyVisibility_Hidden)
        
        context:SetMaterialPropertyVisibility("roughness.factor", MaterialPropertyVisibility_Enabled)
    elseif(not useTexture) then
        context:SetMaterialPropertyVisibility("roughness.useTexture", MaterialPropertyVisibility_Enabled)
        context:SetMaterialPropertyVisibility("roughness.textureMapUv", MaterialPropertyVisibility_Disabled)

        context:SetMaterialPropertyVisibility("roughness.lowerBound", MaterialPropertyVisibility_Disabled)
        context:SetMaterialPropertyVisibility("roughness.upperBound", MaterialPropertyVisibility_Disabled)
        
        context:SetMaterialPropertyVisibility("roughness.factor", MaterialPropertyVisibility_Enabled)
    else
        context:SetMaterialPropertyVisibility("roughness.useTexture", MaterialPropertyVisibility_Enabled)
        context:SetMaterialPropertyVisibility("roughness.textureMapUv", MaterialPropertyVisibility_Enabled)

        context:SetMaterialPropertyVisibility("roughness.lowerBound", MaterialPropertyVisibility_Enabled)
        context:SetMaterialPropertyVisibility("roughness.upperBound", MaterialPropertyVisibility_Enabled)
        
        context:SetMaterialPropertyVisibility("roughness.factor", MaterialPropertyVisibility_Hidden)
    end
end
