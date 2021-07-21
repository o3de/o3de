----------------------------------------------------------------------------------------------------
--
-- Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
-- 
-- SPDX-License-Identifier: Apache-2.0 OR MIT
--
--
--
----------------------------------------------------------------------------------------------------

local OverflowTextAnimate = 
{
    Properties =
    {
        OverflowText = {default = EntityId()},
    },
}

function OverflowTextAnimate:OnActivate()
    self.tickBusHandler = TickBus.Connect(self)
    self.ScriptedEntityTweener = require("Scripts.ScriptedEntityTweener.ScriptedEntityTweener")
    self.ScriptedEntityTweener:OnActivate()
    
    self.timeline = self.ScriptedEntityTweener:TimelineCreate()
end

function OverflowTextAnimate:OnTick(deltaTime, timePoint)
    -- Disconnect from the tick bus
    self.tickBusHandler:Disconnect()
    
    -- Start animating element size on loop
    local elementScaleFactor = 0.6
    local elementWidth = UiTransform2dBus.Event.GetLocalWidth(self.Properties.OverflowText)
    local elementHeight = UiTransform2dBus.Event.GetLocalHeight(self.Properties.OverflowText)
    self.timeline:Add(self.Properties.OverflowText, 0,
        {
            ["w"] = elementWidth,
            ["h"] = elementHeight 
        })
    self.timeline:Add(self.Properties.OverflowText, 3,
        {
            ease = "SineInOut",
            ["w"] = elementWidth * elementScaleFactor,
            ["h"] = elementHeight * elementScaleFactor,
        })
    self.timeline:Add(self.Properties.OverflowText, 3,
        {
            ease = "SineInOut",
            ["w"] = elementWidth,
            ["h"] = elementHeight,
            onComplete = function() self.timeline:Play() end
        })

    -- Start the timeline
    self.timeline:Play()
end

function OverflowTextAnimate:OnDeactivate()
    self.ScriptedEntityTweener:TimelineDestroy(self.timeline)
    self.ScriptedEntityTweener:OnDeactivate()
end

return OverflowTextAnimate
