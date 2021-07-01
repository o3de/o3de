-- 
-- 
-- Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
-- 
-- SPDX-License-Identifier: Apache-2.0 OR MIT
--
-- 

function ScriptTrace(txt)
    Debug.Log(txt)
end

function ScriptExpectTrue(condition, msg)

    if (not condition) then
        ScriptTrace(msg)
    end

end

-- This example shows how to implement a handler for a Script Event that requires an address
-- in order for a handler to be invoked
luaScriptEventWithId = {

    -- This method will be broadcast, but only handlers connected to the matching address 
    -- as the one specified in the event will invoke it
    MethodWithId0 = function(self, param1, param2)
        ScriptTrace("Handler: " ..  tostring(param1) .. " " .. tostring(param2))

        ScriptExpectTrue(typeid(param1) == typeid(0), "Type of param1 must be "..tostring(typeid(0)))
        ScriptExpectTrue(typeid(param2) == typeid(EntityId()), "Type of param2 must be "..tostring(typeid(EntityId())))
        ScriptExpectTrue(param1 == 1, "The first parameter must be 1")
        ScriptExpectTrue(param2 == EntityId(12345), "The received entity Id must match the one sent")

        ScriptTrace("MethodWithId0 handled")

        return true
    end,

    MethodWithId1 = function(self)
        ScriptTrace("MethodWithId1 handled")
    end
}

-- "Script_Event" will be the name of the callable Script Event, it will require the address type to be a string.
local scriptEventDefinition = ScriptEvent("Script_Event", typeid("")) -- Event address is of string type

-- Will define some methods that handlers may implement
local method0 = scriptEventDefinition:AddMethod("MethodWithId0", typeid(false)) -- Return value is Boolean
method0:AddParameter("Param0", typeid(0))
method0:AddParameter("Param1", typeid(EntityId()))

-- NOTE: Type's are specified using the typeid keyword with a VALUE of the type you wish (for example, typeid("EntityId") 
-- will produce the type id for a string, and not the type of EntityId)

scriptEventDefinition:AddMethod("MethodWithId1") -- No return, no parameters

-- Once the Script Event is defined, call Register to enable it, typically this should be done within OnActivate
scriptEventDefinition:Register()

-- At this point, the Script Event is usable, so we will connect a handler to it, this will install luaScriptEventWithId as the Handler
-- which will provide implementations to the methods we defined. Notice that we are connecting with the string "ScriptEventAddress"
-- as the address for this event. Any methods sent to a different address would not be handled by this handler we are connecting.
scriptEventHandler = Script_Event.Connect(luaScriptEventWithId, "ScriptEventAddress")

-- Now we will invoke the event and we will specify "ScriptEventAddress" as the address, this means the handler we previously 
-- connected will be able to handle this event.
local returnValue = Script_Event.Event.MethodWithId0("ScriptEventAddress", 1, EntityId(12345))

-- We know that "Method0" should return true, we verify that it is.
ScriptExpectTrue(returnValue, "Method0's return value must be true")

-- Finally we send "MethodWithdId1" which does not require any parameters, but still needs the address to be provided.
Script_Event.Event.MethodWithId1("ScriptEventAddress")
