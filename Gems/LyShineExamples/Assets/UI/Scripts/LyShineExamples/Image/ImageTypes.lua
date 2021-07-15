----------------------------------------------------------------------------------------------------
--
-- Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
-- 
-- SPDX-License-Identifier: Apache-2.0 OR MIT
--
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
