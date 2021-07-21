----------------------------------------------------------------------------------------------------
--
-- Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
-- 
-- SPDX-License-Identifier: Apache-2.0 OR MIT
--
--
--
----------------------------------------------------------------------------------------------------

local ShowAndInputEnableElementButton = 
{
    Properties = 
    {
        HelpElement = {default = EntityId()},
    },
}

function ShowAndInputEnableElementButton:OnActivate()
    self.buttonHandler = UiButtonNotificationBus.Connect(self, self.entityId)
end

function ShowAndInputEnableElementButton:OnDeactivate()
    self.buttonHandler:Disconnect()
end

function ShowAndInputEnableElementButton:OnButtonClick()
    if UiElementBus.Event.IsEnabled(self.Properties.HelpElement) then
        UiElementBus.Event.SetIsEnabled(self.Properties.HelpElement, false)
        UiInteractableBus.Event.SetIsHandlingEvents(self.Properties.HelpElement, false)
    else
        UiElementBus.Event.SetIsEnabled(self.Properties.HelpElement, true)
        UiInteractableBus.Event.SetIsHandlingEvents(self.Properties.HelpElement, true)
    end
end

return ShowAndInputEnableElementButton
