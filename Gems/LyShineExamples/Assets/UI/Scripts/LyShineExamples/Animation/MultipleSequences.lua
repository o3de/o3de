----------------------------------------------------------------------------------------------------
--
-- Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
-- 
-- SPDX-License-Identifier: Apache-2.0 OR MIT
--
--
--
----------------------------------------------------------------------------------------------------

local MultipleSequences = 
{
    Properties = 
    {
    },
}

function MultipleSequences:OnActivate()
    self.tickBusHandler = TickBus.Connect(self);
end

function MultipleSequences:OnTick(deltaTime, timePoint)
    self.tickBusHandler:Disconnect()

    local canvas = UiElementBus.Event.GetCanvas(self.entityId)

    UiAnimationBus.Event.StartSequence(canvas, "Progress")
    UiAnimationBus.Event.StartSequence(canvas, "Pulse")
    
    UiAnimationBus.Event.StartSequence(canvas, "Progress2")
    UiAnimationBus.Event.StartSequence(canvas, "Pulse2")
end


function MultipleSequences:OnDeactivate()
end

return MultipleSequences
