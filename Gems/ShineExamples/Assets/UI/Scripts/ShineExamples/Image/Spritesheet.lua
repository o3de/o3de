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
