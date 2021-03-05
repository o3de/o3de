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