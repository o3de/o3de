local NestedSpawner = 
{
    Properties = 
    {
		Prefab = { default=SpawnableScriptAssetRef(), description="Prefab to spawn" },
		Translation = { default=Vector3(0, 0, 0) },
		Rotation = { default=Vector3(0, 0, 0) },
		Scale = { default=1 }
    },
}

function NestedSpawner:OnActivate()
	self.spawnableMediator = SpawnableScriptMediator()
	self.ticket = self.spawnableMediator:CreateSpawnTicket(self.Properties.Prefab)
	self.spawnableMediator:SpawnAndParentAndTransform(self.ticket, self.entityId, self.Properties.Translation, self.Properties.Rotation, self.Properties.Scale )
end

return NestedSpawner