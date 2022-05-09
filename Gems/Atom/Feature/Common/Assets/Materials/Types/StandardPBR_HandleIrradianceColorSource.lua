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
    return {"irradianceColorSource", "manualColor"}
end

IrradianceColorSource_BaseColor = 0
IrradianceColorSource_BaseColorTint = 1
IrradianceColorSource_ManualColor = 2

function ProcessEditor(context)
    local irradianceColorSource = context:GetMaterialPropertyValue_enum("irradianceColorSource")

    if ( irradianceColorSource == IrradianceColorSource_BaseColor
      or irradianceColorSource == IrradianceColorSource_BaseColorTint ) then
        context:SetMaterialPropertyVisibility("manualColor", MaterialPropertyVisibility_Hidden)
    elseif (irradianceColorSource == IrradianceColorSource_ManualColor) then
        context:SetMaterialPropertyVisibility("manualColor", MaterialPropertyVisibility_Enabled)
    end
end

