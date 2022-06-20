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
    return {"overrideParallaxSettings", "heightOffset", "heightScale"}
end

function ProcessEditor(context)
    local overrideParallax = context:GetMaterialPropertyValue_bool("overrideParallaxSettings")
    if overrideParallax then
        context:SetMaterialPropertyVisibility("heightOffset", MaterialPropertyVisibility_Enabled)
        context:SetMaterialPropertyVisibility("heightScale", MaterialPropertyVisibility_Enabled)
    else
        context:SetMaterialPropertyVisibility("heightOffset", MaterialPropertyVisibility_Disabled)
        context:SetMaterialPropertyVisibility("heightScale", MaterialPropertyVisibility_Disabled)
    end
end
