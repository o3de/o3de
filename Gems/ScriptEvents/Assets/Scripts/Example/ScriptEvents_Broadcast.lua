-- 
-- 
--  All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
--  its licensors.
-- 
--  For complete copyright and license terms please see the LICENSE at the root of this
--  distribution (the "License"). All use of this software is governed by the License,
--  or, if provided, by the license below or the license accompanying this file. Do not
--  remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
--  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
-- 

function ScriptTrace(txt)
    Debug.Log(txt)
end

function ScriptExpectTrue(condition, msg)

    if (not condition) then
        ScriptTrace(msg)
    end

end

-- This example shows how to implement a handler for a Script Event Broadcast
-- Broadcast Script Events do not specify an address type and so may be handled
-- by connecting to the Script Event.
luaScriptEventBroadcast = {

    -- This method will be called as a result of a Broadcast call on the Script Event.
    BroadcastMethod0 = function(self, param1, param2)
        ScriptTrace("Handler: " ..  tostring(param1) .. " " .. tostring(param2))

        ScriptExpectTrue(typeid(param1) == typeid(0), "Type of param1 must be "..tostring(typeid(0)))
        ScriptExpectTrue(typeid(param2) == typeid(EntityId()), "Type of param2 must be "..tostring(typeid(EntityId())))
        ScriptExpectTrue(param1 == 2, "The first parameter must be 2")
        ScriptExpectTrue(param2 == EntityId(23456), "The received entity Id must match the one sent")

        ScriptTrace("BroadcastMethod0 Called")

        return true
    end,

    BroadcastMethod1 = function(self)
        ScriptTrace("BroadcastMethod1 Called")
    end
}

local scriptEventDefinition = ScriptEvent("Script_Broadcast") -- Script_Broadcast will be the name of the callable Script Event

-- Define methods for Script_Broadcast
local method0 = scriptEventDefinition:AddMethod("BroadcastMethod0", typeid(false)) -- Adding a method expects a method name and an optional return type.
method0:AddParameter("Param0", typeid(0))
method0:AddParameter("Param1", typeid(EntityId()))

-- NOTE: Type's are specified using the typeid keyword with a VALUE of the type you wish (for example, typeid("EntityId") 
-- will produce the type id for a string, and not the type of EntityId)

scriptEventDefinition:AddMethod("BroadcastMethod1")

-- Once the Script Event is defined, call Register to enable it, typically this should be done within OnActivate
scriptEventDefinition:Register()

-- At this point, the Script Event is usable, so we will connect a handler to it, this will install luaScriptEventBroadcast as the Handler
-- which will provide implementations to the methods we defined.
scriptEventHandler = Script_Broadcast.Connect(luaScriptEventBroadcast)

-- In order to test the event, we will Broadcast "BroadcastMethod0" which as defined will return a Boolean value and expects two parameters
local returnValue = Script_Broadcast.Broadcast.BroadcastMethod0(2, EntityId(23456))

-- We know that BroadcastMethod0 should return true, so we will verify the result.
ScriptExpectTrue(returnValue, "BroadcastMethod0's return value must be true")

-- Broadcast an event without a return or parameters, the BroadcastMethod1 will be invoked
Script_Broadcast.Broadcast.BroadcastMethod1()
