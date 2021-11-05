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
    return {"metallic.textureMap", "metallic.useTexture"}
end
 
function GetShaderOptionDependencies()
    return {"o_metallic_useTexture"}
end

function Process(context)
    local textureMap = context:GetMaterialPropertyValue_Image("metallic.textureMap")
    local useTexture = context:GetMaterialPropertyValue_bool("metallic.useTexture")
    context:SetShaderOptionValue_bool("o_metallic_useTexture", useTexture and textureMap ~= nil)
end

function ProcessEditor(context)
    local textureMap = context:GetMaterialPropertyValue_Image("metallic.textureMap")
    local useTexture = context:GetMaterialPropertyValue_bool("metallic.useTexture")

    if(nil == textureMap) then
        context:SetMaterialPropertyVisibility("metallic.useTexture", MaterialPropertyVisibility_Hidden)
        context:SetMaterialPropertyVisibility("metallic.textureMapUv", MaterialPropertyVisibility_Hidden)
        context:SetMaterialPropertyVisibility("metallic.factor", MaterialPropertyVisibility_Enabled)
    elseif(not useTexture) then
        context:SetMaterialPropertyVisibility("metallic.useTexture", MaterialPropertyVisibility_Enabled)
        context:SetMaterialPropertyVisibility("metallic.textureMapUv", MaterialPropertyVisibility_Disabled)
        context:SetMaterialPropertyVisibility("metallic.factor", MaterialPropertyVisibility_Enabled)
    else
        context:SetMaterialPropertyVisibility("metallic.useTexture", MaterialPropertyVisibility_Enabled)
        context:SetMaterialPropertyVisibility("metallic.textureMapUv", MaterialPropertyVisibility_Enabled)
        context:SetMaterialPropertyVisibility("metallic.factor", MaterialPropertyVisibility_Hidden)
    end
end
