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

-- This functor hides the properties for disabled material layers.

function GetMaterialPropertyDependencies()
    return { 
        "blend.enableLayer2",
        "blend.enableLayer3"
    }
end

function SetLayerVisibility(context, layerNamePrefix, isVisible)

    local visibility = MaterialPropertyGroupVisibility_Enabled
    if(not isVisible) then
        visibility = MaterialPropertyGroupVisibility_Hidden
    end
    
    context:SetMaterialPropertyGroupVisibility(layerNamePrefix .. "baseColor", visibility)
    context:SetMaterialPropertyGroupVisibility(layerNamePrefix .. "metallic", visibility)
    context:SetMaterialPropertyGroupVisibility(layerNamePrefix .. "roughness", visibility)
    context:SetMaterialPropertyGroupVisibility(layerNamePrefix .. "specularF0", visibility)
    context:SetMaterialPropertyGroupVisibility(layerNamePrefix .. "normal", visibility)
    context:SetMaterialPropertyGroupVisibility(layerNamePrefix .. "clearCoat", visibility)
    context:SetMaterialPropertyGroupVisibility(layerNamePrefix .. "occlusion", visibility)
    context:SetMaterialPropertyGroupVisibility(layerNamePrefix .. "emissive", visibility)
    context:SetMaterialPropertyGroupVisibility(layerNamePrefix .. "parallax", visibility)
    context:SetMaterialPropertyGroupVisibility(layerNamePrefix .. "uv", visibility)
end

function ProcessEditor(context)
    local enableLayer2 = context:GetMaterialPropertyValue_bool("blend.enableLayer2")
    local enableLayer3 = context:GetMaterialPropertyValue_bool("blend.enableLayer3")
    
    SetLayerVisibility(context, "layer2_", context:GetMaterialPropertyValue_bool("blend.enableLayer2"))
    SetLayerVisibility(context, "layer3_", context:GetMaterialPropertyValue_bool("blend.enableLayer3"))
end
