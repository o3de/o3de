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

local SimpleSpawnDespawn = 
{
    Properties = 
    {
        Prefab = {default=SpawnableScriptAssetRef(), description="Prefab to spawn"},
        DespawnTime={default=3, description="When to despawn the entity"},
    },
}

function SimpleSpawnDespawn:OnActivate()
    self.spawnableMediator = SpawnableScriptMediator()
    self.ticket = self.spawnableMediator:CreateSpawnTicket(self.Properties.Prefab)
    self.spawnableMediator:SpawnAndParentAndTransform(self.ticket, self.entityId, Vector3(0.0,0.0,5.0), Vector3(0.0,0.0,90.0), 5.0 )
    self.spawnableNotificationsBusHandler = SpawnableScriptNotificationsBus.Connect(self, self.ticket:GetId())
end

function SimpleSpawnDespawn:OnTick(deltaTime, timePoint)
    self.timeRemaining = self.timeRemaining-deltaTime
    if (self.timeRemaining < 0) then
        self.tickBusHandler:Disconnect()
        self.spawnableMediator:Despawn(self.ticket)
    end
end

function SimpleSpawnDespawn:OnSpawn(spawnTicket, entityList)
    self.timeRemaining = self.Properties.DespawnTime
    self.tickBusHandler = TickBus.Connect(self)
end

function SimpleSpawnDespawn:OnDespawn(spawnTicket)
    self.spawnableNotificationsBusHandler:Disconnect()    
    ConsoleRequestBus.Broadcast.ExecuteConsoleCommand([[quit]])
end

return SimpleSpawnDespawn