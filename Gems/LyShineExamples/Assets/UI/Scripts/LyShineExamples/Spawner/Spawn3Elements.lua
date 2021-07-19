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

local Spawn3Elements = 
{
    Properties = 
    {
        SpawnerElement = {default = EntityId()},
    },
}

function Spawn3Elements:OnActivate()
    self.buttonHandler = UiButtonNotificationBus.Connect(self, self.entityId)
    self.spawnsPending = 0
    self.spawnTicket1 = nil
    self.spawnTicket2 = nil
    self.spawnTicket3 = nil
end

function Spawn3Elements:OnDeactivate()
    self.buttonHandler:Disconnect()
end

function Spawn3Elements:OnButtonClick()
    if self.spawnTicket1 == nil and self.spawnTicket2 == nil and self.spawnTicket3 == nil then
        self.spawnerHandler = UiSpawnerNotificationBus.Connect(self, self.Properties.SpawnerElement)
        self.spawnTicket1 = UiSpawnerBus.Event.Spawn(self.Properties.SpawnerElement)
        self.spawnTicket2 = UiSpawnerBus.Event.Spawn(self.Properties.SpawnerElement)
        self.spawnTicket3 = UiSpawnerBus.Event.Spawn(self.Properties.SpawnerElement)
        self.spawnsPending = 3
    end
end

function Spawn3Elements:OnTopLevelEntitiesSpawned(ticket, ids)

    if ticket == self.spawnTicket1 or ticket == self.spawnTicket2 or ticket == self.spawnTicket3 then
        local color = Color(0, 0, 0)
        if ticket == self.spawnTicket1 then
            color = Color(1, 0, 0)
        elseif ticket == self.spawnTicket2 then
            color = Color(0, 1, 0)
        elseif ticket == self.spawnTicket3 then
            color = Color(0, 0, 1)
        end
        
        for i = 1,#ids do
            local parent = UiElementBus.Event.GetParent(ids[i])
            if parent == self.Properties.SpawnerElement then
                UiImageBus.Event.SetColor(ids[i], color)
            end
        end
        
        self.spawnsPending = self.spawnsPending - 1
    
        if self.spawnsPending == 0 then
            self.spawnerHandler:Disconnect()
            self.spawnTicket1 = nil
            self.spawnTicket2 = nil
            self.spawnTicket3 = nil
        end
    end
end

function Spawn3Elements:OnSpawnFailed(ticket)
    if ticket == self.spawnTicket1 or ticket == self.spawnTicket2 or ticket == self.spawnTicket3 then
        self.spawnsPending = self.spawnsPending - 1
    
        if self.spawnsPending == 0 then
            self.spawnerHandler:Disconnect()
            self.spawnTicket1 = nil
            self.spawnTicket2 = nil
            self.spawnTicket3 = nil
        end
    end
end

return Spawn3Elements
