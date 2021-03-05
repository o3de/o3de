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
    return {"parallax.enable", "parallax.textureMap"}
end

function GetShaderOptionDependencies()
    return {"o_parallax_feature_enabled", "o_useDepthMap"}
end
 
function Process(context)
    local enable = context:GetMaterialPropertyValue_bool("parallax.enable")
    local textureMap = context:GetMaterialPropertyValue_image("parallax.textureMap")
    context:SetShaderOptionValue_bool("o_parallax_feature_enabled", enable)
    context:SetShaderOptionValue_bool("o_useDepthMap", enable and textureMap ~= nil)
end

function ProcessEditor(context)
    local enable = context:GetMaterialPropertyValue_bool("parallax.enable")
    
    if enable then
        context:SetMaterialPropertyVisibility("parallax.textureMap", MaterialPropertyVisibility_Enabled)
    else 
        context:SetMaterialPropertyVisibility("parallax.textureMap", MaterialPropertyVisibility_Hidden)
    end
    
    local textureMap = context:GetMaterialPropertyValue_image("parallax.textureMap")
    local visibility = MaterialPropertyVisibility_Enabled
    if(not enable or textureMap == nil) then
        visibility = MaterialPropertyVisibility_Hidden
    end

    context:SetMaterialPropertyVisibility("parallax.factor", visibility)
    context:SetMaterialPropertyVisibility("parallax.invert", visibility)
    context:SetMaterialPropertyVisibility("parallax.algorithm", visibility)
    context:SetMaterialPropertyVisibility("parallax.quality", visibility)
    context:SetMaterialPropertyVisibility("parallax.pdo", visibility)
    context:SetMaterialPropertyVisibility("parallax.textureMapUv", visibility)
end
