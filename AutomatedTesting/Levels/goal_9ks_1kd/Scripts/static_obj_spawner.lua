-- Just  basic template that you can quickly copy/paste
-- and start writing your component class
local static_obj_spawner = {
    Properties = {
        applyOffsetZ = { default = true, description="If true, each places object will be offet up according to its aabb."},
        waitTimeBeforeFirstSpawn = { default = 6.0, suffix="s", description="How much time to wait before spwanning the first item."},
        spawnRate = { default = 1.0, suffix="items/mode", description="Rate at which entities should be swapned. If spawnModePerFrame is true, this is items/s. If spawnModePerFrame is true, this is items/frame"},
        isSpawnModePerFrame = { default = true, description="If enabled spawns items on a per frame bases instead of each second."},
        maxActiveItems = { default = 10
                         , description="Maximum amount of concurrent static items."},
        spawnables = { default = {SpawnableScriptAssetRef(), }
                     , description="reference to spawnable asset" },
    }
}

function static_obj_spawner:OnActivate()
    -- Debug.Log("static_obj_spawner:OnActivate=" .. tostring(self.Properties.spawnable))
    self._spawnableScriptMediator = SpawnableScriptMediator()

    self._spawnTicketCount, self._ticketIdList, self._spawnTickets = self:_BuildSpawnTicketsList(self.Properties.spawnables)

    self._spawnRate = math.floor(self.Properties.spawnRate)
    assert(self._spawnRate > 0, "Invalid spawn rate")
    self._elapsedTime = 0.0
    self._activeItemsCount = 0
    self._maxActiveItems = self.Properties.maxActiveItems
    self._currentSpawnPos = TransformBus.Event.GetWorldTranslation(self.entityId)
    self._nextPosDirection = 1.0

    require("Levels.goal_9ks_1kd.Scripts.ScriptEvents.StaticObjNotificationBus")

	-- Connect to tick bus to receive time updates, during these ticks
	-- we will dispatch all enqueued events.
    self._onTickState = self._OnTickWaiting
	self.tickBusHandler = TickBus.Connect(self)
end

function static_obj_spawner:OnDeactivate()
    -- Crashes the game/editor.
    -- self._spawnableScriptMediator:Despawn(self._ticket)

    self._spawnableScriptMediator = nil
    self:_DestroySpawnTicketsList()

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

-->>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
-- TickBus  START
-->>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
function static_obj_spawner:OnTick(deltaTime, timePoint)
    self:_onTickState(deltaTime, timePoint)

    --self.tickBusHandler:Disconnect()
    --self.tickBusHandler = nil
end


-- Loading the terrain takes a while. We wait some time before starting
-- to spawn the objects.
function static_obj_spawner:_OnTickWaiting(deltaTime, timePoint)
    self._elapsedTime = self._elapsedTime + deltaTime
    if self._elapsedTime < self.Properties.waitTimeBeforeFirstSpawn then
        return
    end
    Debug.Log("static_obj_spawner:_OnTickWaiting ready to spawn")
    local terrainAabb = TerrainDataRequestBus.Broadcast.GetTerrainAabb()
    local terrainSizeX = terrainAabb:GetXExtent()
    local terrainSizeY = terrainAabb:GetYExtent()
    local numObjects = self._maxActiveItems
    local numColumns = math.sqrt(numObjects)
    local xOverY = terrainSizeX / terrainSizeY
    local numObjectsX = numColumns * xOverY
    local numObjectsY = numColumns / xOverY
    local spaceX = terrainSizeX / numObjectsX
    local spaceY = terrainSizeY / numObjectsY
    local startingPosition = terrainAabb.min
    

    startingPosition.x = startingPosition.x - spaceX
    self._currentSpawnPos = startingPosition

    self._terrainData = {
        min = terrainAabb.min,
        max = terrainAabb.max,
        spaceX = spaceX,
        spaceY = spaceY,
    }
    Debug.Log("static_obj_spawner:_OnTickWaiting aabb min max" .. tostring(self._terrainData.min) .. ", " .. tostring(self._terrainData.max))
    Debug.Log("static_obj_spawner:_OnTickWaiting spaceX=" .. tostring(spaceX) .. ", spaceY=" .. tostring(spaceY))

    self._elapsedTime = 0
    if self.Properties.isSpawnModePerFrame then
        self._onTickState = self._OnTickSpawningPerFrame
    else
        self._onTickState = self._OnTickSpawningPerSecond
    end
end


-- Called per tick when the user wants to spawn objects each second.
function static_obj_spawner:_OnTickSpawningPerSecond(deltaTime, timePoint)
    self._elapsedTime = self._elapsedTime + deltaTime
    if self._elapsedTime < 1.0 then
        return
    end
    self._elapsedTime = 0.0

    local available = self.Properties.maxActiveItems - self._activeItemsCount

    if available < 1 then
        Debug.Log("static_obj_spawner:_OnTickSpawning Completed the creation of all static objects")
        self.tickBusHandler:Disconnect()
        self.tickBusHandler = nil
        StaticObjNotificationBus.Broadcast.OnAllStaticEntitiesCreated(self._activeItemsCount)
        return
    end

    local numItemsToSpawn = math.min(self._spawnRate, available)
    for idx=1,numItemsToSpawn do
        local nextTicketIndex = self:_GetNextTicketIndex()
        --Debug.Log("static_obj_spawner:_OnTickSpawning nextTicketIndex = " .. tostring(nextTicketIndex))
        self:_Spawn(nextTicketIndex)
    end

end

-- Called per tick when the user wants to spawn objects each frame.
function static_obj_spawner:_OnTickSpawningPerFrame(deltaTime, timePoint)
    local available = self.Properties.maxActiveItems - self._activeItemsCount

    if available < 1 then
        Debug.Log("static_obj_spawner:_OnTickSpawning Completed the creation of all static objects")
        self.tickBusHandler:Disconnect()
        self.tickBusHandler = nil
        StaticObjNotificationBus.Broadcast.OnAllStaticEntitiesCreated(self._activeItemsCount)
        return
    end

    local numItemsToSpawn = math.min(self._spawnRate, available)
    for idx=1,numItemsToSpawn do
        local nextTicketIndex = self:_GetNextTicketIndex()
        --Debug.Log("static_obj_spawner:_OnTickSpawning nextTicketIndex = " .. tostring(nextTicketIndex))
        self:_Spawn(nextTicketIndex)
    end

end



--<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
-- TickBus END
--<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<


-->>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
-- SpawnableScriptNotificationsBus  START
-->>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
function static_obj_spawner:OnSpawn(spawnTicket, entityList)

    --if self._ticket:GetId() ~= spawnTicket:GetId() then
    --    Debug.Log("static_obj_spawner:OnSpawn Got event from invalid ticket")
    --    return
    --end
    --Debug.Log("static_obj_spawner:OnSpawn Got event from ticketId=" .. tostring(spawnTicket:GetId()))

    -- Here we'll store the dummy parent entity that comes with the prefab system.
    -- We'll unparent the first and only child and remove this dummy entity.
    local dummyParent = EntityId()
    local entityToUnparent = EntityId()

    -- Debug.Log("static_obj_spawner:OnSpawn Got the following list with " .. tostring(size) .. " entities")
    local size = entityList:Size()
    for idx=1, size do
        local entityId = entityList:At(idx)
        if not dummyParent:IsValid() then
            local parentId = TransformBus.Event.GetParentId(entityId)
            if  not parentId:IsValid() then
                dummyParent = entityId 
                local children = TransformBus.Event.GetChildren(dummyParent)
                entityToUnparent = children:At(1)
            end
        end
        local name = GameEntityContextRequestBus.Broadcast.GetEntityName(entityId)
        local pos = TransformBus.Event.GetWorldTranslation(entityId)
        --TransformBus.Event.SetWorldTranslation(entityId, pos)
        --local pos = TransformBus.Event.GetWorldTranslation(entityId)
        -- Debug.Log("static_obj_spawner:OnSpawn entityId=" .. tostring(entityId) .. ", name=" .. name .. ", pos=" .. tostring(pos))
    end

    assert(dummyParent:IsValid() and entityToUnparent:IsValid(), "The dummy parent and entityToUnparent must be valid")

    -- now, let's unparent the child from the dummy parent:
    local pos = self:_GetNextSpawnPos()
    self:_UnparentAndRelocate(dummyParent, entityToUnparent, pos)

    StaticObjNotificationBus.Broadcast.OnStaticEntitySpawned(entityToUnparent)

    -- GameEntityContextRequestBus.Broadcast.GetEntityName
    --for entityId in entityList do
    --    Debug.Log("static_obj_spawner:OnSpawn entityId=" .. tostring(entityId))
    --end
end

function static_obj_spawner:OnDespawn(spawnTicket)
    --if self._ticket:GetId() ~= spawnTicket:GetId() then
    --    --Debug.Log("static_obj_spawner:OnDespawn Got event from invalid ticket")
    --    return
    --end
    Debug.Log("static_obj_spawner:OnDespawn despawn complete")
end

--<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
-- SpawnableScriptNotificationsBus END
--<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<


-->>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
-- Private methods  START
-->>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
function static_obj_spawner:_BuildSpawnTicketsList(spawnablesAssetRefList)
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
            Debug.Log("static_obj_spawner:_BuildSpawnTicketsList index=" .. tostring(count) .. ", ticketId=" .. tostring(ticketId))
        else
            Debug.Warning(false, "Failed to create spawm ticket at index=" .. tostring(idx))
        end
    end
    return count, retList, retTable
end

function static_obj_spawner:_DestroySpawnTicketsList()
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


-- Returns a tuple nextTicketIndex, spawnPos
function static_obj_spawner:_GetNextTicketIndex()
    local nextTicketIndex = self._nextTicketIndex or 1
    if nextTicketIndex > self._spawnTicketCount then
        nextTicketIndex = 1
    end

    self._nextTicketIndex = nextTicketIndex + 1
    return nextTicketIndex
end


-- Returns the position where the new spawned object should be located,
-- also updates that position for the next time this is called.
function static_obj_spawner:_GetNextSpawnPos()
    local spawnPosX = self._currentSpawnPos.x
    local spawnPosY = self._currentSpawnPos.y
    local deltaX = self._terrainData.spaceX
    local maxX = self._terrainData.max.x
    local minX = self._terrainData.min.x

    spawnPosX = spawnPosX + deltaX
    if spawnPosX > maxX then
        spawnPosX = minX
        spawnPosY = spawnPosY + self._terrainData.spaceY
    end

    -- Get the height at the current x,y
    local spawnPos = Vector3(spawnPosX, spawnPosY, 0.0)
    local tuple_f_b = TerrainDataRequestBus.Broadcast.GetHeight(spawnPos, 0)
    local spawnPosZ = tuple_f_b:Get0()
    local isHole = tuple_f_b:Get1()
    spawnPos.z = spawnPosZ
    --Debug.Log("static_obj_spawner:_GetNextSpawnPos = " .. tostring(spawnPos))
    self._currentSpawnPos = spawnPos
    return spawnPos
end


-- Issues a request to spawn one object, using the SpawnTicket located
-- in the stored array by the @ticketIndex
function static_obj_spawner:_Spawn(ticketIndex)
    -- SpawnAndParentAndTransform(
    --         EntitySpawnTicket spawnTicket,
    --         const AZ::EntityId& parentId,
    --         const AZ::Vector3& translation,
    --         const AZ::Vector3& rotation,
    --         float scale
    --local pos = TransformBus.Event.GetWorldTranslation(self.entityId)
    --Debug.Log("static_obj_spawner:_Spawn ticketIndex=" .. tostring(ticketIndex))
    -- self._spawnableScriptMediator:SpawnAndParentAndTransform(self._ticket, EntityId(), pos, Vector3(0.0, 0.0, 0.0), 1.0)

    local ticketId = self._ticketIdList[ticketIndex]
    local ticketData = self._spawnTickets[ticketId]
    -- self._spawnableScriptMediator:SpawnAndParentAndTransform(ticketData.ticket, EntityId(), spawnLocation, Vector3(0.0, 0.0, 0.0), 1.0)
    self._spawnableScriptMediator:Spawn(ticketData.ticket)

    self._activeItemsCount = self._activeItemsCount + 1
end


-- Helper function that unparents @entityId from @parentId. @parentId is destroyed in the process.
-- This is useful, because, unfortunately, the spawnable api always creates a dummy parent
-- entity that gets in the way.
-- Also @entityId is relocated at newWorldLocation
function static_obj_spawner:_UnparentAndRelocate(parentId, entityId, newWorldLocation)
    SimulatedBodyComponentRequestBus.Event.DisablePhysics(entityId)
    TransformBus.Event.SetParent(entityId, EntityId())
    GameEntityContextRequestBus.Broadcast.DestroyGameEntityAndDescendants(parentId)

    if self.Properties.applyOffsetZ then
        local aabb = ShapeComponentRequestsBus.Event.GetEncompassingAabb(entityId)
        -- Debug.Log("static_obj_spawner:_Unparent aabb=" .. tostring(aabb))
        newWorldLocation.z = newWorldLocation.z - aabb.min.z
    end

    --Debug.Log("static_obj_spawner:_Unparent New pos = " .. tostring(pos))
    TransformBus.Event.SetWorldTranslation(entityId, newWorldLocation)
    SimulatedBodyComponentRequestBus.Event.EnablePhysics(entityId)
end
--<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
-- Private methods END
--<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<


return static_obj_spawner
