-- Just  basic template that you can quickly copy/paste
-- and start writing your component class
local ui_objects_score = {
    Properties = {
        staticObjCounterId = { default = EntityId()
            , description="The entity with text element for the static objects counter." },
        dynamicActiveObjCounterId = { default = EntityId()
            , description="The entity with text element for the active dynamic objects counter." },
        spawnedObjCounterId = { default = EntityId()
            , description="The entity with text element for the total amount of spawned objects since the simulation started." },
        despawnedObjCounterId = { default = EntityId()
            , description="The entity with text element for the total amount of despawned objects since the simulation started." },
    }
}

function ui_objects_score:OnActivate()
    assert(EntityId.IsValid(self.Properties.staticObjCounterId), "ui_objects_score:OnActivate. Invalid <staticObjCounterId> entity.")
    assert(EntityId.IsValid(self.Properties.dynamicActiveObjCounterId), "ui_objects_score:OnActivate. Invalid <dynamicActiveObjCounterId> entity.")
    assert(EntityId.IsValid(self.Properties.spawnedObjCounterId), "ui_objects_score:OnActivate. Invalid <spawnedObjCounterId> entity.")
    assert(EntityId.IsValid(self.Properties.despawnedObjCounterId), "ui_objects_score:OnActivate. Invalid <despawnedObjCounterId> entity.")

    self._staticObjCount = 0
    self._dynamicActiveObjCount = 0
    self._spawnedObjCount = 0
    self._despawnedObjCount = 0

    require("Levels.goal_9ks_1kd.Scripts.ScriptEvents.DynamicObjTriggerNotificationBus")
    self.DynamicObjTriggerNotificationBusEventHandler = DynamicObjTriggerNotificationBus.Connect(self)

    require("Levels.goal_9ks_1kd.Scripts.ScriptEvents.StaticObjNotificationBus")
    self.StaticObjNotificationBusEventHandler = StaticObjNotificationBus.Connect(self)

end

function ui_objects_score:OnDeactivate()
    if self.DynamicObjTriggerNotificationBusEventHandler then
        self.DynamicObjTriggerNotificationBusEventHandler:Disconnect()
    end

    if self.StaticObjNotificationBusEventHandler then
        self.StaticObjNotificationBusEventHandler:Disconnect()
    end

end

-->>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
-- Public methods  START
-->>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>


--<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
-- Public methods END
--<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



-->>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
-- Private methods  START
-->>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>


--<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
-- Private methods END
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
function ui_objects_score:OnEntityEntered(entityId)
    self._dynamicActiveObjCount = self._dynamicActiveObjCount + 1
    self._spawnedObjCount = self._spawnedObjCount + 1

    UiTextBus.Event.SetText(self.Properties.dynamicActiveObjCounterId, tostring(self._dynamicActiveObjCount))
    UiTextBus.Event.SetText(self.Properties.spawnedObjCounterId, tostring(self._spawnedObjCount))
end

--  An entity exited the simulation volume.
-- OnEntityExited typeid of input parameters:
-- @param entityId = typeid(EntityId())
-- Returns nil
function ui_objects_score:OnEntityExited(entityId)
    self._dynamicActiveObjCount = self._dynamicActiveObjCount - 1
    self._despawnedObjCount = self._despawnedObjCount + 1

    UiTextBus.Event.SetText(self.Properties.dynamicActiveObjCounterId, tostring(self._dynamicActiveObjCount))
    UiTextBus.Event.SetText(self.Properties.despawnedObjCounterId, tostring(self._despawnedObjCount))
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
function ui_objects_score:OnStaticEntitySpawned(entityId)
    self._staticObjCount = self._staticObjCount + 1
    self._spawnedObjCount = self._spawnedObjCount + 1

    UiTextBus.Event.SetText(self.Properties.staticObjCounterId, tostring(self._staticObjCount))
    UiTextBus.Event.SetText(self.Properties.spawnedObjCounterId, tostring(self._spawnedObjCount))
end

-- Completed the creation of all static entities.
-- OnAllStaticEntitiesCreated typeid of input parameters:
-- @param totalCount = typeid(1.0)
-- Returns nil
function ui_objects_score:OnAllStaticEntitiesCreated(totalCount)
end

--<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
-- StaticObjNotificationBus Override END
--<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

return ui_objects_score
