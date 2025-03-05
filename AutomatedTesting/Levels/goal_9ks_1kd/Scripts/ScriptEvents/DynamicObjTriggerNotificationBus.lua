-- This ScriptEvent file was auto generated with ScriptEvent_Transpiler.
-- Source Available at https://github.com/lumbermixalot/ScriptEvent_Transpiler.git
local dynamicObjTriggerNotificationBus = ScriptEvent("DynamicObjTriggerNotificationBus")

-- How the ScriptEvent lua file is generated
-- E:\GIT\ScriptEvent_Transpiler> python main.py -e DynamicObjTriggerNotificationBus -t D:\GIT\o3de\AtomSampleViewer\Scripts\Static9k_Dynamic1k\ScriptEvents\IDL\DynamicObjTriggerNotificationBus.luaidl D:\GIT\o3de\AtomSampleViewer\Scripts\Static9k_Dynamic1k\ScriptEvents
--   
-- The DynamicObjTriggerNotificationBus broadcasts notifications whenever an entity enters or exits the world
-- simulation volume.
--     
--  An entity entered the simulation volume.
local methodOnEntityEntered = dynamicObjTriggerNotificationBus:AddMethod("OnEntityEntered")
methodOnEntityEntered:AddParameter("entityId", typeid(EntityId()))

--  An entity exited the simulation volume.
local methodOnEntityExited = dynamicObjTriggerNotificationBus:AddMethod("OnEntityExited")
methodOnEntityExited:AddParameter("entityId", typeid(EntityId()))

dynamicObjTriggerNotificationBus:Register()