----------------------------------------------------------------------------------------------------
--
-- Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
-- 
-- SPDX-License-Identifier: Apache-2.0 OR MIT
--
--
--
----------------------------------------------------------------------------------------------------

local PlayAnimationOnStart = 
{
    Properties = 
    {
        AnimName = {default = ""},
    },
}

function PlayAnimationOnStart:OnActivate()
    self.tickBusHandler = TickBus.Connect(self);
end

function PlayAnimationOnStart:OnTick(deltaTime, timePoint)
    self.tickBusHandler:Disconnect()

    self.canvas = UiElementBus.Event.GetCanvas(self.entityId)
    
    -- Start the idle button sequence
    UiAnimationBus.Event.StartSequence(self.canvas, self.Properties.AnimName)
end


function PlayAnimationOnStart:OnDeactivate()
end

return PlayAnimationOnStart
