----------------------------------------------------------------------------------------------------
--
-- Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
-- 
-- SPDX-License-Identifier: Apache-2.0 OR MIT
--
--
--
----------------------------------------------------------------------------------------------------

local MoveBallUp = 
{
    Properties = 
    {
        Ball = {default = EntityId()},
        MaxOffset = {default = 0},
    },
}

function MoveBallUp:OnActivate()
    self.buttonHandler = UiButtonNotificationBus.Connect(self, self.entityId)
end

function MoveBallUp:OnButtonClick()
    local offsets = UiTransform2dBus.Event.GetOffsets(self.Properties.Ball)
    local height = offsets.bottom - offsets.top
    offsets.top = offsets.top - 50
    offsets.bottom = offsets.bottom - 50
    if (offsets.top < self.Properties.MaxOffset) then
        offsets.top = self.Properties.MaxOffset
        offsets.bottom = offsets.top + height
    end
    UiTransform2dBus.Event.SetOffsets(self.Properties.Ball, offsets)
end

function MoveBallUp:OnDeactivate()
    self.buttonHandler:Disconnect()
end

return MoveBallUp
