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

local StylingMarkupLinkText = 
{
    Properties = 
    {
        ClickDataText = {default = EntityId()},
    },
}

function StylingMarkupLinkText:OnActivate()
    self.markupButtonNotificationBusHandler = UiMarkupButtonNotificationsBus.Connect(self, self.entityId);
    self.ScriptedEntityTweener = require("Scripts.ScriptedEntityTweener.ScriptedEntityTweener")
    self.ScriptedEntityTweener:OnActivate()
    self.timeline = self.ScriptedEntityTweener:TimelineCreate()

    self.initializationHandler = UiInitializationBus.Connect(self, self.entityId);
end

function StylingMarkupLinkText:InGamePostActivate()
    self.initializationHandler:Disconnect()
    self.initializationHandler = nil

    -- The clickable link text description will scale out of view (down to zero scale)
    -- via animation.
    self.timeline:Add(self.Properties.ClickDataText, 0,
        {
            ["scaleX"] = 1.0,
            ["scaleY"] = 1.0 
        })
    self.timeline:Add(self.Properties.ClickDataText, 1,
        {
            delay = 2.0,
            ease = "SineInOut",
            ["scaleX"] = 0.0,
            ["scaleY"] = 0.0,
        })
end

function StylingMarkupLinkText:OnClick(linkId, action, data)
    UiTextBus.Event.SetText(self.Properties.ClickDataText, "Link clicked: action = " .. action .. ", data = " .. data)

    -- Start the timeline
    self.timeline:Play()

end


function StylingMarkupLinkText:OnDeactivate()
    self.markupButtonNotificationBusHandler:Disconnect()
    self.ScriptedEntityTweener:TimelineDestroy(self.timeline)
    self.ScriptedEntityTweener:OnDeactivate()
end

return StylingMarkupLinkText
