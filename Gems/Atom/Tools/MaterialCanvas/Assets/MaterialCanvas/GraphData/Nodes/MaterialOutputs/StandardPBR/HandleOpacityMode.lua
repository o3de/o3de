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
    return {"general.opacity_mode"}
end
 
OpacityMode_Opaque = 0
OpacityMode_Cutout = 1
OpacityMode_Blended = 2
OpacityMode_TintedTransparent = 3

function Process(context)

    local opacityMode = OpacityMode_Opaque
    if context:HasMaterialProperty("general.opacity_mode") then
        opacityMode = context:GetMaterialPropertyValue_enum("general.opacity_mode")
    end
    
    context:SetInternalMaterialPropertyValue_bool("hasPerPixelClip", opacityMode == OpacityMode_Cutout)
    context:SetInternalMaterialPropertyValue_bool("isTransparent", opacityMode == OpacityMode_Blended)
    context:SetInternalMaterialPropertyValue_bool("isTintedTransparent", opacityMode == OpacityMode_TintedTransparent)
end
