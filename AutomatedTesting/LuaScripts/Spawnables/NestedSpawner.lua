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

local NestedSpawner = 
{
    Properties = 
    {
        Prefab = { default=SpawnableScriptAssetRef(), description="Prefab to spawn" },
        Translation = { default=Vector3(0.0, 0.0, 0.0) },
        Rotation = { default=Vector3(0.0, 0.0, 0.0) },
        Scale = { default=1.0 }
    },
}

function NestedSpawner:OnActivate()
    self.spawnableMediator = SpawnableScriptMediator()
    self.ticket = self.spawnableMediator:CreateSpawnTicket(self.Properties.Prefab)
    self.spawnableMediator:SpawnAndParentAndTransform(self.ticket, self.entityId, self.Properties.Translation, self.Properties.Rotation, self.Properties.Scale )
end

return NestedSpawner