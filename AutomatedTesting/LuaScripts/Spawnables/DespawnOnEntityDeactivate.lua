local DespawnOnEntityDeactivate = 
{
    Properties = 
    {
		Prefab = { default=SpawnableScriptAssetRef(), description="Prefab to spawn" }
    },
}

function DespawnOnEntityDeactivate:OnActivate()
	self.spawnableMediator = SpawnableScriptMediator()
	self.ticket = self.spawnableMediator:CreateSpawnTicket(self.Properties.Prefab)
	self.spawnableNotificationsBusHandler = SpawnableScriptNotificationsBus.Connect(self, self.ticket:GetId())
	self.spawnableMediator:Spawn(self.ticket)
end

function DespawnOnEntityDeactivate:OnSpawn(spawnTicket, entityList)
	self.spawnableNotificationsBusHandler:Disconnect()
	self.entityBusNotificationsHandler = EntityBus.Connect(self, entityList[1])
	GameEntityContextRequestBus.Broadcast.DeactivateGameEntity(entityList[1])
end

function DespawnOnEntityDeactivate:OnEntityDeactivated(entityId)
	self.spawnableMediator:Despawn(self.ticket)
end

return DespawnOnEntityDeactivate