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
    return {"ambientOcclusion.enable", "ambientOcclusion.textureMap"}
end

function GetShaderOptionDependencies()
    return {"o_ambientOcclusion_useTexture"}
end
 
function Process(context)
    local enableAo = context:GetMaterialPropertyValue_bool("ambientOcclusion.enable")
    local textureMap = context:GetMaterialPropertyValue_image("ambientOcclusion.textureMap")

    context:SetShaderOptionValue_bool("o_ambientOcclusion_useTexture", enableAo and textureMap ~= nil)
end

function ProcessEditor(context)
    local enableAo = context:GetMaterialPropertyValue_bool("ambientOcclusion.enable")

    if(not enableAo) then
        context:SetMaterialPropertyVisibility("ambientOcclusion.factor", MaterialPropertyVisibility_Hidden)
        context:SetMaterialPropertyVisibility("ambientOcclusion.textureMap", MaterialPropertyVisibility_Hidden)
        context:SetMaterialPropertyVisibility("ambientOcclusion.textureMapUv", MaterialPropertyVisibility_Hidden)
    else
        context:SetMaterialPropertyVisibility("ambientOcclusion.textureMap", MaterialPropertyVisibility_Enabled)
        local textureMap = context:GetMaterialPropertyValue_image("ambientOcclusion.textureMap")
        if(textureMap == nil) then
            context:SetMaterialPropertyVisibility("ambientOcclusion.factor", MaterialPropertyVisibility_Hidden)
            context:SetMaterialPropertyVisibility("ambientOcclusion.textureMapUv", MaterialPropertyVisibility_Hidden)
        else
            context:SetMaterialPropertyVisibility("ambientOcclusion.factor", MaterialPropertyVisibility_Enabled)
            context:SetMaterialPropertyVisibility("ambientOcclusion.textureMapUv", MaterialPropertyVisibility_Enabled)
        end
    end
end
