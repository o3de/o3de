-- Just  basic template that you can quickly copy/paste
-- and start writing your component class
local dynamic_obj_spawner = {
    Properties = {
        spacingBetweenEachSpawnedObject = { default = 2.0, suffix="m", description="Spacing in X axis between each spawned object."},
        waitTimeBeforeFirstSpawn = { default = 0.0, suffix="s", description="How much time to wait before spwaning the first item. If it is <= 0 then will wait for OnAllStaticEntitiesCreated event before spawning the first item"},
        spawnRate = { default = 1.0, suffix="items/mode", description="Rate at which entities should be swapned. If spawnModePerFrame is true, this is items/s. If spawnModePerFrame is true, this is items/frame"},
        isSpawnModePerFrame = { default = true, description="If enabled spawns items on a per frame bases instead of each second."},
        maxActiveItems = { default = 10
                         , description="Maximum amount of concurrent active items."},
        spawnables = { default = {SpawnableScriptAssetRef(), }
                     , description="reference to spawnable asset" },
    }
}

function dynamic_obj_spawner:OnActivate()
    -- Debug.Log("dynamic_obj_spawner:OnActivate=" .. tostring(self.Properties.spawnable))
    self._spawnableScriptMediator = SpawnableScriptMediator()

    self._spawnTicketCount, self._ticketIdList, self._spawnTickets = self:_BuildSpawnTicketsList(self.Properties.spawnables)

    self._spawnRate = math.floor(self.Properties.spawnRate)
    assert(self._spawnRate > 0, "Invalid spawn rate")
    self._elapsedTime = 0.0
    self._totalSpawnedItemsCount = 0
    self._activeItemsCount = 0
    self._maxActiveItems = self.Properties.maxActiveItems
    self._currentSpawnPos = TransformBus.Event.GetWorldTranslation(self.entityId)

    require("Levels.goal_9ks_1kd.Scripts.ScriptEvents.DynamicObjTriggerNotificationBus")
    self.DynamicObjTriggerNotificationBusEventHandler = DynamicObjTriggerNotificationBus.Connect(self)

    require("Levels.goal_9ks_1kd.Scripts.ScriptEvents.StaticObjNotificationBus")
    self.StaticObjNotificationBusEventHandler = StaticObjNotificationBus.Connect(self)

    self._onTickState = self._OnTickWaiting

    if self.Properties.waitTimeBeforeFirstSpawn >= 0.1 then
        self.tickBusHandler = TickBus.Connect(self)
    end
end

function dynamic_obj_spawner:OnDeactivate()
    -- Crashes the game/editor.
    -- self._spawnableScriptMediator:Despawn(self._ticket)

    self._spawnableScriptMediator = nil
    self:_DestroySpawnTicketsList()

    if self.DynamicObjTriggerNotificationBusEventHandler then
        self.DynamicObjTriggerNotificationBusEventHandler:Disconnect()
    end

    if self.StaticObjNotificationBusEventHandler then
        self.StaticObjNotificationBusEventHandler:Disconnect()
    end

	if self.tickBusHandler then
		self.tickBusHandler:Disconnect()
	end

end


-->>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
-- Public methods  START
-->>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

--<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
-- Public methods END
--<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

-->>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
-- DynamicObjTriggerNotificationBus Override START
-->>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

-- How the ScriptEvent lua file is generated
-- E:\GIT\ScriptEvent_Transpiler> python main.py -e DynamicObjTriggerNotificationBus -t D:\GIT\o3de\AtomSampleViewer\Scripts\Static9k_Dynamic1k\ScriptEvents\IDL\DynamicObjTriggerNotificationBus.luaidl D:\GIT\o3de\AtomSampleViewer\Scripts\Static9k_Dynamic1k\ScriptEvents
--   
-- The DynamicObjTriggerNotificationBus broadcasts notifications whenever an entity enters or exits the world
-- simulation volume.
--     
--  An entity entered the simulation volume.
-- OnEntityEntered typeid of input parameters:
-- @param entityId = typeid(EntityId())
-- Returns nil
function dynamic_obj_spawner:OnEntityEntered(entityId)
end

--  An entity exited the simulation volume.
-- OnEntityExited typeid of input parameters:
-- @param entityId = typeid(EntityId())
-- Returns nil
function dynamic_obj_spawner:OnEntityExited(entityId)
    -- Debug.Log("dynamic_obj_spawner:OnEntityExited. Will destroy entityId=" .. tostring(entityId))
    GameEntityContextRequestBus.Broadcast.DestroyGameEntityAndDescendants(entityId)
    self._activeItemsCount = self._activeItemsCount - 1
end

--<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
-- DynamicObjTriggerNotificationBus Override END
--<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<


-->>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
-- StaticObjNotificationBus Override START
-->>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

-- How the ScriptEvent lua file is generated
-- E:\GIT\ScriptEvent_Transpiler> python main.py -e StaticObjNotificationBus -t D:\GIT\o3de\AutomatedTesting\Levels\goal_9ks_1kd\Scripts\ScriptEvents\IDL\StaticObjNotificationBus.luaidl D:\GIT\o3de\AutomatedTesting\Levels\goal_9ks_1kd\Scripts\ScriptEvents
--   
-- The StaticObjNotificationBus broadcasts notifications whenever a static object is spawned.
--     
--  An entity was created
-- OnStaticEntitySpawned typeid of input parameters:
-- @param entityId = typeid(EntityId())
-- Returns nil
function dynamic_obj_spawner:OnStaticEntitySpawned(entityId)
end

-- Completed the creation of all static entities.
-- OnAllStaticEntitiesCreated typeid of input parameters:
-- @param totalCount = typeid(1.0)
-- Returns nil
function dynamic_obj_spawner:OnAllStaticEntitiesCreated(totalCount)
    if self.tickBusHandler then
        return
    end

    Debug.Log("dynamic_obj_spawner:OnAllStaticEntitiesCreated. Will start spawning objects because all " .. tostring(totalCount) .. " static objects were created")

    -- Fake time so _OnTickWaiting only runs once.
    self._elapsedTime = self.Properties.waitTimeBeforeFirstSpawn + 1
    self._onTickState = self._OnTickWaiting

    self.tickBusHandler = TickBus.Connect(self)
end

--<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
-- StaticObjNotificationBus Override END
--<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

-->>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
-- TickBus  START
-->>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
function dynamic_obj_spawner:OnTick(deltaTime, timePoint)
    self:_onTickState(deltaTime, timePoint)

    --self.tickBusHandler:Disconnect()
    --self.tickBusHandler = nil
end


-- Loading the terrain takes a while. We wait some time before starting
-- to spawn the objects.
function dynamic_obj_spawner:_OnTickWaiting(deltaTime, timePoint)
    self._elapsedTime = self._elapsedTime + deltaTime
    if self._elapsedTime < self.Properties.waitTimeBeforeFirstSpawn then
        return
    end

    Debug.Log("dynamic_obj_spawner:_OnTickWaiting ready to spawn")
    local terrainAabb = TerrainDataRequestBus.Broadcast.GetTerrainAabb()
    self._terrainData = {
        min = terrainAabb.min,
        max = terrainAabb.max,
    }
    Debug.Log("dynamic_obj_spawner:_OnTickWaiting aabb min max" .. tostring(self._terrainData.min) .. ", " .. tostring(self._terrainData.max))
    self._elapsedTime = 0
    if self.Properties.isSpawnModePerFrame then
        self._onTickState = self._OnTickSpawningPerFrame
    else
        self._onTickState = self._OnTickSpawningPerSecond
    end
end


-- Called per tick when the user wants to spawn objects each second.
function dynamic_obj_spawner:_OnTickSpawningPerSecond(deltaTime, timePoint)
    self._elapsedTime = self._elapsedTime + deltaTime
    if self._elapsedTime < 1.0 then
        return
    end
    self._elapsedTime = 0.0

    local available = self.Properties.maxActiveItems - self._activeItemsCount
    local numItemsToSpawn = math.min(self._spawnRate, available)
    for idx=1,numItemsToSpawn do
        local nextTicketIndex = self:_GetNextTicketIndex()
        --Debug.Log("dynamic_obj_spawner:_OnTickSpawning nextTicketIndex = " .. tostring(nextTicketIndex))
        self:_Spawn(nextTicketIndex)
    end

end


-- Called per tick when the user wants to spawn objects each frame.
function dynamic_obj_spawner:_OnTickSpawningPerFrame(deltaTime, timePoint)
    local available = self.Properties.maxActiveItems - self._activeItemsCount
    local numItemsToSpawn = math.min(self._spawnRate, available)
    for idx=1,numItemsToSpawn do
        local nextTicketIndex = self:_GetNextTicketIndex()
        --Debug.Log("dynamic_obj_spawner:_OnTickSpawning nextTicketIndex = " .. tostring(nextTicketIndex))
        self:_Spawn(nextTicketIndex)
    end
end




--<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
-- TickBus END
--<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<


-->>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
-- SpawnableScriptNotificationsBus  START
-->>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
function dynamic_obj_spawner:OnSpawn(spawnTicket, entityList)
    -- Debug.Log("dynamic_obj_spawner:OnSpawn Got event from ticketId=" .. tostring(spawnTicket:GetId()))

    -- Here we'll store the dummy parent entity that comes with the prefab system.
    -- We'll unparent the first and only child and remove this dummy entity.
    local dummyParent = EntityId()
    local entityToUnparent = EntityId()

    -- Debug.Log("dynamic_obj_spawner:OnSpawn Got the following list with " .. tostring(size) .. " entities")
    -- let's find the dummy parent entity.
    local size = entityList:Size()
    for idx=1, size do
        local entityId = entityList:At(idx)
        if not dummyParent:IsValid() then
            local parentId = TransformBus.Event.GetParentId(entityId)
            if  not parentId:IsValid() then
                dummyParent = entityId
                local children = TransformBus.Event.GetChildren(dummyParent)
                entityToUnparent = children:At(1)
                -- Debug.Log("dynamic_obj_spawner:OnSpawn dummyParent=" .. tostring(entityId))
            end
        end
        -- local name = GameEntityContextRequestBus.Broadcast.GetEntityName(entityId)
        -- local pos = TransformBus.Event.GetWorldTranslation(entityId)
        --TransformBus.Event.SetWorldTranslation(entityId, pos)
        --local pos = TransformBus.Event.GetWorldTranslation(entityId)
        -- Debug.Log("dynamic_obj_spawner:OnSpawn entityId=" .. tostring(entityId) .. ", name=" .. name .. ", pos=" .. tostring(pos))
    end

    assert(dummyParent:IsValid() and entityToUnparent:IsValid(), "The dummy parent and entityToUnparent must be valid")

    local pos = self:_GetNextSpawnPos()
    self:_UnparentAndRelocate(dummyParent, entityToUnparent, pos)

end

function dynamic_obj_spawner:OnDespawn(spawnTicket)
    --if self._ticket:GetId() ~= spawnTicket:GetId() then
    --    --Debug.Log("dynamic_obj_spawner:OnDespawn Got event from invalid ticket")
    --    return
    --end
    Debug.Log("dynamic_obj_spawner:OnDespawn despawn complete")
end

--<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
-- SpawnableScriptNotificationsBus END
--<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<


-->>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
-- Private methods  START
-->>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

-- Called during initialization to get the list of prefabs that will be spawned.
-- Returns a tuple (count, retList, retTable)  where:
--     count: How many items in the list.
--     retList: A table that works as an array starting at index 1. The value at each
--              position in the array is the ticked id (an integer defined by the Spawnable runtime.)
--     retTable: A table that work as a dictionary.
--               Key is the ticket id.
--               Value is the EntitySpawnTicket()
function dynamic_obj_spawner:_BuildSpawnTicketsList(spawnablesAssetRefList)
    local count = 0
    local retList = {}
    local retTable = {}
    for idx, spawnableScriptAssetRef in ipairs(spawnablesAssetRefList) do
        local ticket = self._spawnableScriptMediator:CreateSpawnTicket(spawnableScriptAssetRef)
        if ticket then
            local ticketId = ticket:GetId()
            retTable[ticketId] = {}
            retTable[ticketId].ticket = ticket
            retTable[ticketId].handler = SpawnableScriptNotificationsBus.Connect(self, ticketId)
            count = count + 1
            retList[count] = ticketId
            Debug.Log("dynamic_obj_spawner:_BuildSpawnTicketsList index=" .. tostring(count) .. ", ticketId=" .. tostring(ticketId))
        else
            Debug.Warning(false, "Failed to create spawm ticket at index=" .. tostring(idx))
        end
    end
    return count, retList, retTable
end

-- Called during deactivation  to destroy spawn tickets and related data.
function dynamic_obj_spawner:_DestroySpawnTicketsList()
    if not self._spawnTickets then
        return
    end

    for ticketId, dataTable in pairs(self._spawnTickets) do
        dataTable.ticket = nil
        if dataTable.handler then
            dataTable.handler:Disconnect()
            dataTable.handler = nil
        end
    end

    self._spawnTickets = nil
end


-- helper function that Returns the next Ticket Index
function dynamic_obj_spawner:_GetNextTicketIndex()
    local nextTicketIndex = self._nextTicketIndex or 1
    if nextTicketIndex > self._spawnTicketCount then
        nextTicketIndex = 1
    end

    self._nextTicketIndex = nextTicketIndex + 1
    return nextTicketIndex
end


-- Returns the position where the new spawned object should be located,
-- also updates that position for the next time this is called.
function dynamic_obj_spawner:_GetNextSpawnPos()
    local spawnPos = self._currentSpawnPos
    local deltaX = self.Properties.spacingBetweenEachSpawnedObject
    local maxX = self._terrainData.max.x - deltaX
    local minX = self._terrainData.min.x + deltaX

    spawnPos = spawnPos + Vector3(deltaX, 0, 0)
    if spawnPos.x > maxX then
        spawnPos.x = minX
    end
    self._currentSpawnPos = spawnPos
    return spawnPos
end


-- Issues a request to spawn one object, using the SpawnTicket located
-- in the stored array by the @ticketIndex
function dynamic_obj_spawner:_Spawn(ticketIndex)
    local ticketId = self._ticketIdList[ticketIndex]
    local ticketData = self._spawnTickets[ticketId]
    -- self._spawnableScriptMediator:SpawnAndParentAndTransform(ticketData.ticket, EntityId(), spawnLocation, Vector3(0.0, 0.0, 0.0), 1.0)
    self._spawnableScriptMediator:Spawn(ticketData.ticket)

    self._totalSpawnedItemsCount = self._totalSpawnedItemsCount + 1
    self._activeItemsCount = self._activeItemsCount + 1
end


-- Helper function that unparents @entityId from @parentId. @parentId is destroyed in the process.
-- This is useful, because, unfortunately, the spawnable api always creates a dummy parent
-- entity that gets in the way.
-- Also @entityId is relocated at newWorldLocation
function dynamic_obj_spawner:_UnparentAndRelocate(parentId, entityId, newWorldLocation)
    RigidBodyRequestBus.Event.DisablePhysics(entityId)
    TransformBus.Event.SetParent(entityId, EntityId())
    GameEntityContextRequestBus.Broadcast.DestroyGameEntity(parentId)

    --Debug.Log("dynamic_obj_spawner:_Unparent New pos = " .. tostring(newWorldLocation))
    TransformBus.Event.SetWorldTranslation(entityId, newWorldLocation)
    RigidBodyRequestBus.Event.EnablePhysics(entityId)
end
--<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
-- Private methods END
--<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<


return dynamic_obj_spawner
