----------------------------------------------------------------------------------------------------
--
-- Copyright (c) Contributors to the Open 3D Engine Project.
-- For complete copyright and license terms please see the LICENSE at the root of this distribution.
--
-- SPDX-License-Identifier: Apache-2.0 OR MIT
--
--
--
----------------------------------------------------------------------------------------------------

local SetUseAlphaGradientCheckbox = 
{
    Properties = 
    {
        Element = {default = EntityId()},
    },
}

function SetUseAlphaGradientCheckbox:OnActivate()
    self.checkboxHandler = UiCheckboxNotificationBus.Connect(self, self.entityId)
end

function SetUseAlphaGradientCheckbox:OnDeactivate()
    self.checkboxHandler:Disconnect()
end

function SetUseAlphaGradientCheckbox:OnCheckboxStateChange(isChecked)
    UiMaskBus.Event.SetUseRenderToTexture(self.Properties.Element, isChecked)
end

return SetUseAlphaGradientCheckbox
