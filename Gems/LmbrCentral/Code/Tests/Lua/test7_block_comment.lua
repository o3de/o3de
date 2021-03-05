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
local SpawnerScriptSample = 
{
    Properties =
    {
        ScriptVar = { default = "aaaa" }
    }
}

function SpawnerScriptSample:OnActivate()
    -- Register our handlers to receive notification from the spawner attached to this entity.
    if( self.spawnerNotiBusHandler == nil ) then
        self.spawnerNotiBusHandler = SpawnerComponentNotificationBus.Connect(self, self.entityId)
    end
end

-- This handler is called when we start spawning a slice.
function SpawnerScriptSample:OnSpawnBegin(sliceTicket)
    -- Do something so we know if/when this is being called
    
    -- UNIT TEST: The dependencies in the following block comment should not be logged by the LuaBuilder
    --[[ConsoleRequestBus.Broadcast.ExecuteConsoleCommand("exec somefile.cfg")
    ConsoleRequestBus.Broadcast.ExecuteConsoleCommand("exec somefile/in/a/folder.cfg")--]]
    
    Debug.Log("Slice Spawn Begin")
end

-- This handler is called when we're finished spawning a slice.
function SpawnerScriptSample:OnSpawnEnd(sliceTicket)
    -- Do something so we know if/when this is being called
    Debug.Log("Slice Spawn End")
end

-- This handler is called whenever an entity is spawned.
function SpawnerScriptSample:OnEntitySpawned(sliceTicket, entityId)
    -- Do something so we know if/when this is being called
    Debug.Log("Entity Spawned: " .. tostring(entityId) )
end

function SpawnerScriptSample:OnDeactivate()
    -- Disconnect our spawner notificaton 
    if self.spawnerNotiBusHandler ~= nil then
        self.spawnerNotiBusHandler:Disconnect()
        self.spawnerNotiBusHandler = nil
    end
end

return SpawnerScriptSample