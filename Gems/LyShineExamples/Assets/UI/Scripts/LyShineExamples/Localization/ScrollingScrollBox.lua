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

local ScrollingScrollBox = 
{
    Properties = 
    {
        toggleButton = {default = EntityId()},
        scrollSpeed = 75.0,
    },
}

function ScrollingScrollBox:OnActivate()
    -- Connect to the toggle button to know when to toggle the scrolling on/off
    self.buttonHandler = UiButtonNotificationBus.Connect(self, self.Properties.toggleButton)
    -- Connect to the tick bus to be able to scroll
    self.tickHandler = TickBus.Connect(self, 0)

    self:InitAutomatedTestEvents()
end

function ScrollingScrollBox:OnDeactivate()
    -- Deactivate from the button bus
    self.buttonHandler:Disconnect()
    -- Deactivate from the tick bus only if we were still connected
    if (self.tickHandler ~= nil) then
        self.tickHandler:Disconnect()
    end

    self:DeInitAutomatedTestEvents()
end

function ScrollingScrollBox:OnTick(dt)
    -- Scroll down according to speed and dt
    local scrollOffset = UiScrollBoxBus.Event.GetScrollOffset(self.entityId)
    scrollOffset.y = scrollOffset.y - self.Properties.scrollSpeed * dt
    UiScrollBoxBus.Event.SetScrollOffset(self.entityId, scrollOffset)
end

function ScrollingScrollBox:OnButtonClick()
    -- If we are currently connected to the tick bus
    if (self.tickHandler ~= nil) then
        -- Disconnect from the tick bus to toggle the scrolling off
        self.tickHandler:Disconnect()
        self.tickHandler = nil
    -- Else if we are not currently connected to the tick bus
    else
        -- Connect to the tick bus to toggle the scrolling on
        self.tickHandler = TickBus.Connect(self, self.entityId)
    end
end

-- [Automated Testing] setup
function ScrollingScrollBox:InitAutomatedTestEvents()
    self.automatedTestSetScrollValueId = GameplayNotificationId(EntityId(), "AutomatedTestScrollValue", "float");
    self.automatedTestSetScrollHandler = GameplayNotificationBus.Connect(self, self.automatedTestSetScrollValueId);
end

-- [Automated Testing] event handling
function ScrollingScrollBox:OnEventBegin(value)
    if (GameplayNotificationBus.GetCurrentBusId() == self.automatedTestSetScrollValueId) then
        -- Toggle scrolling off first, then you can set the scroll amount
        local scrollOffset = UiScrollBoxBus.Event.GetScrollOffset(self.entityId)
        scrollOffset.y = value
        UiScrollBoxBus.Event.SetScrollOffset(self.entityId, scrollOffset)
    end
end

-- [Automated Testing] teardown
function ScrollingScrollBox:DeInitAutomatedTestEvents()
    if (self.automatedTestSetScrollHandler ~= nil) then
        self.automatedTestSetScrollHandler:Disconnect();
        self.automatedTestSetScrollHandler = nil;
    end
end

return ScrollingScrollBox