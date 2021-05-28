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

local SpawnElements = 
{
    Properties = 
    {
        SpawnerElement = {default = EntityId()},
    },
}

function SpawnElements:OnActivate()
    self.buttonHandler = UiButtonNotificationBus.Connect(self, self.entityId)
    self.spawnTicket = nil
end

function SpawnElements:OnDeactivate()
    self.buttonHandler:Disconnect()
end

function SpawnElements:OnButtonClick()
    -- don't spawn another dynamic slice until we have processed the last one
    if self.spawnTicket == nil then
        self.spawnerHandler = UiSpawnerNotificationBus.Connect(self, self.Properties.SpawnerElement)
        self.spawnTicket = UiSpawnerBus.Event.Spawn(self.Properties.SpawnerElement)
    end
end

function SpawnElements:OnEntitySpawned(ticket, id)
    if ticket == self.spawnTicket then
        -- this could be the image element or the child text element.
        -- But we just set all images to yellow and all texts to black.
        -- Alternatively we could use OnTopLevelEntitiesSpawned and
        -- just get the image element and then get its child.
        UiImageBus.Event.SetColor(id, Color(1, 1, 0))
        UiTextBus.Event.SetColor(id, Color(0, 0, 0))
    end
end

function SpawnElements:OnSpawnEnd(ticket)
    if ticket == self.spawnTicket then
        self.spawnerHandler:Disconnect()
        self.spawnTicket = nil
    end
end

function SpawnElements:OnSpawnFailed(ticket)
    if ticket == self.spawnTicket then
        self.spawnerHandler:Disconnect()
        self.spawnTicket = nil
    end
end

return SpawnElements
