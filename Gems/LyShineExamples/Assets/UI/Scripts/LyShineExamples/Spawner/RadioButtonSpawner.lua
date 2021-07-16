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

local RadioButtonSpawner = 
{
    Properties = 
    {
        SpawnerElement = {default = EntityId()},
        ColorsButton = {default = EntityId()},
        ShapesButton = {default = EntityId()},
        PatternsButton = {default = EntityId()},
    },
}

function RadioButtonSpawner:OnActivate()
    self.interactableHandler = UiRadioButtonGroupNotificationBus.Connect(self, self.entityId)
    self.spawnsPending = 0
    
    -- connect to the tickbus in order to set the right badio button group correctly for the
    -- starting selection on the left radio button group. We do this on the next frame after everything
    -- has been activated.    
    self.tickBusHandler = TickBus.Connect(self)
end

function RadioButtonSpawner:OnDeactivate()
    self.interactableHandler:Disconnect()
end

function RadioButtonSpawner:OnTick()
    local activeButton = UiRadioButtonGroupBus.Event.GetState(self.entityId)
    self:SpawnRadioButtons(activeButton)
    
    self.tickBusHandler:Disconnect()
end

function RadioButtonSpawner:OnRadioButtonGroupStateChange(activeButton)
    self:SpawnRadioButtons(activeButton)
end

function RadioButtonSpawner:OnTopLevelEntitiesSpawned(ticket, ids)
    -- we could do this in OnSpawnEnd but this is a convenient way to add each top-level element
    -- spawned to be part of the radio button group. There is only one top-level element in the
    -- dynamic slice and we know it is the radio button
    for i = 1,#ids do
        local parent = UiElementBus.Event.GetParent(ids[i])
        if parent == self.Properties.SpawnerElement then
            UiRadioButtonGroupBus.Event.AddRadioButton(self.Properties.SpawnerElement, ids[i])
        end
    end
end

function RadioButtonSpawner:OnSpawnEnd(ticket)

    self.spawnsPending = self.spawnsPending - 1

    if self.spawnsPending == 0 then
        self.spawnerHandler:Disconnect()
        
        -- The spawns are completed
        local activeButton = UiRadioButtonGroupBus.Event.GetState(self.entityId)
        
        -- get the children that were spawned
    
        local children = UiElementBus.Event.GetChildren(self.Properties.SpawnerElement)

        if activeButton == self.Properties.ColorsButton then
            if #children == 4 then
                self:SetRadioButtonText(children[1], "Red")
                self:SetRadioButtonText(children[2], "Green")
                self:SetRadioButtonText(children[3], "Blue")
                self:SetRadioButtonText(children[4], "Yellow")
            end
        elseif activeButton == self.Properties.ShapesButton then
            if #children == 3 then
                self:SetRadioButtonText(children[1], "Circle")
                self:SetRadioButtonText(children[2], "Triangle")
                self:SetRadioButtonText(children[3], "Square")
            end
        elseif activeButton == self.Properties.PatternsButton then
            if #children == 2 then
                self:SetRadioButtonText(children[1], "Stripes")
                self:SetRadioButtonText(children[2], "Crosshatch")
            end
        end
                    
        -- set the first one in the group to be active
        if #children > 0 then
            UiRadioButtonGroupBus.Event.SetState(self.Properties.SpawnerElement, children[1], true)
        end
    end
end

function RadioButtonSpawner:OnSpawnFailed(ticket)
    self.spawnsPending = self.spawnsPending - 1

    if self.spawnsPending == 0 then
        self.spawnerHandler:Disconnect()
    end
end

function RadioButtonSpawner:SpawnRadioButtons(activeButton)
    if self.spawnsPending == 0 then
        -- remove the existing children of the SpawnerElement (the right radio button group)
        local children = UiElementBus.Event.GetChildren(self.Properties.SpawnerElement)
        if children then
            for i = 1,#children do
                UiElementBus.Event.DestroyElement(children[i])
            end
        end
    
        -- depending on which button is active in the left radio button group, we spawn the required
        -- number of radio buttons. They will automatically become children of the SpawnerElement
        -- (the right radio button group) but not automatically part of the radio button group.
        local numButtonsToSpawn = 0
        if activeButton == self.Properties.ColorsButton then
            numButtonsToSpawn = 4
        elseif activeButton == self.Properties.ShapesButton then
            numButtonsToSpawn = 3
        elseif activeButton == self.Properties.PatternsButton then
            numButtonsToSpawn = 2
        end

        if numButtonsToSpawn > 0 then
            self.spawnerHandler = UiSpawnerNotificationBus.Connect(self, self.Properties.SpawnerElement)
        end
        
        -- spawn the required number of radio buttons
        for i = 1,numButtonsToSpawn do
            UiSpawnerBus.Event.Spawn(self.Properties.SpawnerElement)
        end
        
        self.spawnsPending = numButtonsToSpawn
    end
end

-- Helper function to set the text on a child component of the radio button.
-- We know that the radio button element in our dynamic slice has a child called "Text"
function RadioButtonSpawner:SetRadioButtonText(rb, value)
    local textEntity = UiElementBus.Event.FindChildByName(rb, "Text")
    if (textEntity and textEntity:IsValid()) then
        UiTextBus.Event.SetText(textEntity, value)
    end
end

return RadioButtonSpawner
