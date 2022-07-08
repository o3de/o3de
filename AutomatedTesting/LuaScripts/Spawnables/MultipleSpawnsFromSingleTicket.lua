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

local MultipleSpawnsFromSingleTicket = 
{
    Properties = 
    {
        Prefab = { default=SpawnableScriptAssetRef(), description="Prefab to spawn" },
        SpawnCount = { default=3, description="How many prefabs to spawn" },
        Offset = { default=Vector3(0.0, 2.0, 0.0), description="Translation offset for each spawn" }
    },
}

function MultipleSpawnsFromSingleTicket:OnActivate()
    self.entityCount = 0
    self.spawnsRemaining = self.Properties.SpawnCount
    self.translation = Vector3(0.0, 0.0, 0.0)
    self.spawnableMediator = SpawnableScriptMediator()
    self.ticket = self.spawnableMediator:CreateSpawnTicket(self.Properties.Prefab)
    self.spawnableNotificationsBusHandler = SpawnableScriptNotificationsBus.Connect(self, self.ticket:GetId())
    self:Spawn()
end

function MultipleSpawnsFromSingleTicket:OnSpawn(spawnTicket, entityList)
    self.entityCount = self.entityCount + entityList.GetSize(entityList)
    if (self.spawnsRemaining > 0)
    then
        self:Spawn()
    else
        if (self.entityCount == 6)
        then
            self.spawnableNotificationsBusHandler:Disconnect()    
            ConsoleRequestBus.Broadcast.ExecuteConsoleCommand([[quit]])
        end
    end
end

function MultipleSpawnsFromSingleTicket:Spawn()
    self.spawnsRemaining = self.spawnsRemaining - 1
    self.translation = self.translation + self.Properties.Offset
    self.spawnableMediator:SpawnAndParentAndTransform(self.ticket, self.entityId, self.translation, Vector3(0.0, 0.0, 0.0), 1.0 )
end

return MultipleSpawnsFromSingleTicket