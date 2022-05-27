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