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

local ImageTypes = 
{
    Properties = 
    {
        ShowOutlinesCheckbox = {default = EntityId()},
        Outlines = { default = { EntityId(), EntityId(), EntityId(), EntityId(), EntityId(), EntityId(), EntityId() } },
    },
}

function ImageTypes:OnActivate()
    self.showOutlinesCBHandler = UiCheckboxNotificationBus.Connect(self, self.Properties.ShowOutlinesCheckbox)

    self.tickBusHandler = TickBus.Connect(self);
end

function ImageTypes:OnTick(deltaTime, timePoint)
    self.tickBusHandler:Disconnect()

    -- Initialize outlines
    self:ShowOutlines(false)
end

function ImageTypes:OnDeactivate()
    self.showOutlinesCBHandler:Disconnect()
end

function ImageTypes:OnCheckboxStateChange(isChecked)
    if (UiCheckboxNotificationBus.GetCurrentBusId() == self.Properties.ShowOutlinesCheckbox) then
        self:ShowOutlines(isChecked)
    end
end

function ImageTypes:ShowOutlines(show)
    for i = 0, #self.Properties.Outlines do
        UiElementBus.Event.SetIsEnabled(self.Properties.Outlines[i], show)
    end
end

return ImageTypes