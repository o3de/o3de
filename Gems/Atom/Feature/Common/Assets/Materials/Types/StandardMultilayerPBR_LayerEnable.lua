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
    
    SetLayerVisibility(context, "layer2.", context:GetMaterialPropertyValue_bool("blend.enableLayer2"))
    SetLayerVisibility(context, "layer3.", context:GetMaterialPropertyValue_bool("blend.enableLayer3"))
end
