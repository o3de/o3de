-- This class template file was auto generated with ScriptEvent_Transpiler.
-- Source Available at https://github.com/lumbermixalot/ScriptEvent_Transpiler.git
local StaticObjNotificationBus_subclass = {
    Properties = {
    },
}


function StaticObjNotificationBus_subclass:OnActivate()

    require("Scripts.ScriptEvents.StaticObjNotificationBus")
    self.StaticObjNotificationBusEventHandler = StaticObjNotificationBus.Connect(self)

end

function StaticObjNotificationBus_subclass:OnDeactivate()

    if self.StaticObjNotificationBusEventHandler then
        self.StaticObjNotificationBusEventHandler:Disconnect()
    end

end

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
function StaticObjNotificationBus_subclass:OnStaticEntitySpawned(entityId)
end

-- Completed the creation of all static entities.
-- OnAllStaticEntitiesCreated typeid of input parameters:
-- @param totalCount = typeid(1.0)
-- Returns nil
function StaticObjNotificationBus_subclass:OnAllStaticEntitiesCreated(totalCount)
end

--<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
-- StaticObjNotificationBus Override END
--<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

return StaticObjNotificationBus_subclass
