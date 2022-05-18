local SimpleSpawnNoDespawn = 
{
    Properties = 
    {
		Prefab = { default=SpawnableScriptAssetRef(), description="Prefab to spawn" }
    },
}

function SimpleSpawnNoDespawn:OnActivate()
	self.spawnableMediator = SpawnableScriptMediator()
	self.ticket = self.spawnableMediator:CreateSpawnTicket(self.Properties.Prefab)
	self.spawnableMediator:Spawn(self.ticket)
end

return SimpleSpawnNoDespawn