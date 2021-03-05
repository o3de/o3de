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