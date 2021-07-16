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

local ChildMaskElement = 
{
    Properties = 
    {
    },
}

function ChildMaskElement:OnActivate()
    self.tickBusHandler = TickBus.Connect(self);

    self:InitAutomatedTestEvents()
end

function ChildMaskElement:OnTick(deltaTime, timePoint)
    self.tickBusHandler:Disconnect()

    local canvas = UiElementBus.Event.GetCanvas(self.entityId)

    UiAnimationBus.Event.StartSequence(canvas, "Animate")
end


function ChildMaskElement:OnDeactivate()
    self:DeInitAutomatedTestEvents()
end

-- [Automated Testing] setup
function ChildMaskElement:InitAutomatedTestEvents()
    self.automatedTestStopMaskAnimateId = GameplayNotificationId(EntityId(), "AutomatedTestStopMaskAnimate", "float");
    self.automatedTestStopMaskAnimateHandler = GameplayNotificationBus.Connect(self, self.automatedTestStopMaskAnimateId);
end

-- [Automated Testing] event handling
function ChildMaskElement:OnEventBegin(value)
    if (GameplayNotificationBus.GetCurrentBusId() == self.automatedTestStopMaskAnimateId) then
        local canvas = UiElementBus.Event.GetCanvas(self.entityId)
        UiAnimationBus.Event.StopSequence(canvas, "Animate")
        UiAnimationBus.Event.ResetSequence(canvas, "Animate")
    end
end

-- [Automated Testing] teardown
function ChildMaskElement:DeInitAutomatedTestEvents()
    if (self.automatedTestStopMaskAnimateHandler ~= nil) then
        self.automatedTestStopMaskAnimateHandler:Disconnect();
        self.automatedTestStopMaskAnimateHandler = nil;
    end
end

return ChildMaskElement
