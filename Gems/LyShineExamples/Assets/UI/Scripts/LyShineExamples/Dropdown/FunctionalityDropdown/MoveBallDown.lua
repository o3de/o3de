----------------------------------------------------------------------------------------------------
--
-- All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
-- its licensors.
--
-- For complete copyright and license terms please see the LICENSE at the root of this
-- distribution (the "License"). All use of this software is governed by the License,
-- or, if provided, by the license below or the license accompanying this file. Do not
-- remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
-- WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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