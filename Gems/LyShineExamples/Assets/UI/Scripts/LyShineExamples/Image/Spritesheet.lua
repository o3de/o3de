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

local Spritesheet = 
{
    Properties = 
    {
        ButtonText = {default = EntityId()},
        MainPanel = {default = EntityId()},
        SpritesheetSourcePanel = {default = EntityId()}
    },
}

function Spritesheet:OnActivate()
    self.tickBusHandler = TickBus.Connect(self)
    self.buttonHandler = UiButtonNotificationBus.Connect(self, self.entityId)
    self.showSpritesheet = false
    
end

function Spritesheet:OnTick(deltaTime, timePoint)
    self.tickBusHandler:Disconnect()

    UiElementBus.Event.SetIsEnabled(self.Properties.SpritesheetSourcePanel, self.showSpritesheet)
end

function Spritesheet:OnButtonClick()
    if (self.showSpritesheet) then
      self.showSpritesheet = false
      UiTextBus.Event.SetText(self.Properties.ButtonText, "Show Spritesheet")
    else
      self.showSpritesheet = true
      UiTextBus.Event.SetText(self.Properties.ButtonText, "Hide Spritesheet")
    end
    
    UiElementBus.Event.SetIsEnabled(self.Properties.SpritesheetSourcePanel, self.showSpritesheet)
    UiElementBus.Event.SetIsEnabled(self.Properties.MainPanel, not self.showSpritesheet)
    
end

function Spritesheet:OnDeactivate()
    self.buttonHandler:Disconnect()
end

return Spritesheet
