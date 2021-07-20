----------------------------------------------------------------------------------------------------
--
-- Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
-- 
-- SPDX-License-Identifier: Apache-2.0 OR MIT
--
--
--
----------------------------------------------------------------------------------------------------

local HideThisElementButton = 
{
    Properties = 
    {
    },
}

function HideThisElementButton:OnActivate()
    self.buttonHandler = UiButtonNotificationBus.Connect(self, self.entityId)
    UiElementBus.Event.SetIsEnabled(self.entityId, false)
    UiInteractableBus.Event.SetIsHandlingEvents(self.entityId, false)
end

function HideThisElementButton:OnDeactivate()
    self.buttonHandler:Disconnect()
end

function HideThisElementButton:OnButtonClick()
    UiElementBus.Event.SetIsEnabled(self.entityId, false)
    UiInteractableBus.Event.SetIsHandlingEvents(self.entityId, false)
end

return HideThisElementButton
