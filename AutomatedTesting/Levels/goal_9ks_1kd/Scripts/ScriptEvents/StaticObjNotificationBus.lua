-- This ScriptEvent file was auto generated with ScriptEvent_Transpiler.
-- Source Available at https://github.com/lumbermixalot/ScriptEvent_Transpiler.git
local staticObjNotificationBus = ScriptEvent("StaticObjNotificationBus")

-- How the ScriptEvent lua file is generated
-- E:\GIT\ScriptEvent_Transpiler> python main.py -e StaticObjNotificationBus -t D:\GIT\o3de\AutomatedTesting\Levels\goal_9ks_1kd\Scripts\ScriptEvents\IDL\StaticObjNotificationBus.luaidl D:\GIT\o3de\AutomatedTesting\Levels\goal_9ks_1kd\Scripts\ScriptEvents
--   
-- The StaticObjNotificationBus broadcasts notifications whenever a static object is spawned.
--     
--  An entity was created
local methodOnStaticEntitySpawned = staticObjNotificationBus:AddMethod("OnStaticEntitySpawned")
methodOnStaticEntitySpawned:AddParameter("entityId", typeid(EntityId()))

-- Completed the creation of all static entities.
local methodOnAllStaticEntitiesCreated = staticObjNotificationBus:AddMethod("OnAllStaticEntitiesCreated")
methodOnAllStaticEntitiesCreated:AddParameter("totalCount", typeid(1.0))

staticObjNotificationBus:Register()