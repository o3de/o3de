----------------------------------------------------------------------------------------------------
--
-- Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
-- 
-- SPDX-License-Identifier: Apache-2.0 OR MIT
--
--
--
----------------------------------------------------------------------------------------------------

local MoveBallDown = 
{
    Properties = 
    {
        Ball = {default = EntityId()},
        MinOffset = {default = 0},
    },
}

function MoveBallDown:OnActivate()
    self.buttonHandler = UiButtonNotificationBus.Connect(self, self.entityId)
end

function MoveBallDown:OnButtonClick()
    local offsets = UiTransform2dBus.Event.GetOffsets(self.Properties.Ball)
    local height = offsets.bottom - offsets.top
    offsets.top = offsets.top + 50
    offsets.bottom = offsets.bottom + 50
    if (offsets.bottom > self.Properties.MinOffset) then
        offsets.bottom = self.Properties.MinOffset
        offsets.top = offsets.bottom - height
    end
    UiTransform2dBus.Event.SetOffsets(self.Properties.Ball, offsets)
end

function MoveBallDown:OnDeactivate()
    self.buttonHandler:Disconnect()
end

return MoveBallDown
