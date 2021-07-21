----------------------------------------------------------------------------------------------------
--
-- Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
-- 
-- SPDX-License-Identifier: Apache-2.0 OR MIT
--
--
--
----------------------------------------------------------------------------------------------------

local DestroyBall = 
{
    Properties = 
    {
        Ball = {default = EntityId()},
    },
}

function DestroyBall:OnActivate()
    self.buttonHandler = UiButtonNotificationBus.Connect(self, self.entityId)
end

function DestroyBall:OnButtonClick()
    UiElementBus.Event.SetIsEnabled(self.Properties.Ball, false)
end

function DestroyBall:OnDeactivate()
    self.buttonHandler:Disconnect()
end

return DestroyBall
