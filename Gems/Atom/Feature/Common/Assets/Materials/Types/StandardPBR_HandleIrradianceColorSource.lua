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
    return {"irradiance.irradianceColorSource", "irradiance.manualColor"}
end
 
IrradianceColorSource_BaseColor = 0
IrradianceColorSource_ManualColor = 1

function ProcessEditor(context)
    local irradianceColorSource = context:GetMaterialPropertyValue_enum("irradiance.irradianceColorSource")
    
    if(irradianceColorSource == IrradianceColorSource_BaseColor) then
        context:SetMaterialPropertyVisibility("irradiance.manualColor", MaterialPropertyVisibility_Hidden)
    elseif(irradianceColorSource == IrradianceColorSource_ManualColor) then
        context:SetMaterialPropertyVisibility("irradiance.manualColor", MaterialPropertyVisibility_Enabled)
    end
end

