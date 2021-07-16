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

local SetTextFromInput = 
{
    Properties = 
    {
        TextToSet = {default = EntityId()},
    },
}

function SetTextFromInput:OnActivate()
    self.textInputHandler = UiTextInputNotificationBus.Connect(self, self.entityId)
end

function SetTextFromInput:OnDeactivate()
    self.textInputHandler:Disconnect()
end

function SetTextFromInput:OnTextInputEndEdit(textString)
    UiTextBus.Event.SetText(self.Properties.TextToSet, textString)
    UiTextInputBus.Event.SetText(self.entityId, "")
end

return SetTextFromInput
