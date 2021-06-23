----------------------------------------------------------------------------------------------------
--
-- Copyright (c) Contributors to the Open 3D Engine Project
-- 
-- SPDX-License-Identifier: Apache-2.0 OR MIT
--
--
--
----------------------------------------------------------------------------------------------------

local ColorBall = 
{
    Properties = 
    {
        Ball = {default = EntityId()},
        Color = {default = Color(1,1,1)},
    },
}

function ColorBall:OnActivate()
    self.buttonHandler = UiButtonNotificationBus.Connect(self, self.entityId)
end

function ColorBall:OnButtonClick()
    UiImageBus.Event.SetColor(self.Properties.Ball, self.Properties.Color)
end

function ColorBall:OnDeactivate()
    self.buttonHandler:Disconnect()
end

return ColorBall
