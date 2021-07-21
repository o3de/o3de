----------------------------------------------------------------------------------------------------
--
-- Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
-- 
-- SPDX-License-Identifier: Apache-2.0 OR MIT
--
--
--
----------------------------------------------------------------------------------------------------

--This utility function allows you to easily create handlers when needing to handle multiple IDs. 
--Common examples are inputs (where you have multiple key presses/actions you need to handle) and 
--gameplay notifications (where there's several gameplay events your component needs to handle). 
--See GameplayUtils.lua and InputUtils.lua as examples.

-- require with:
-- local multiHandlers = require('scripts.utils.components.multihandlers')
-- create multihandlers like:
-- return {
--     ConnectMultiHandlers = multiHandlers(GameplayNotificationBus, GameplayEventNotificationId),
-- }

local function ConnectMultiHandlers(bus, idType)
    -- Generate listener constructor
    return function(eventTable)
        -- Our object emulating an ebus handler
        local proxyHandler = {
            handlers = { }
        }
        -- Connect to the bus for each id/handler passed
        for busId, handlers in pairs(eventTable) do
            Debug.Error(typeid(busId) == typeid(idType), "Wrong event id type, expected " .. tostring(typeid(busId)) .. " but got " .. tostring(typeid(idType)))

            -- Setup proxy table 
            proxyHandler.handlers[busId] = {
                originalHandlerTable = handlers;    -- Reference to original handler table passed
                handlerTable = { }                  -- Table containing actual handler functions
            }
            local currentProxy = proxyHandler.handlers[busId]

            -- For each event handler, wrap and copy to proxy handlerTable
            for eventName, eventHandler in pairs(handlers) do
                if (type(eventHandler) == 'function') then
                    -- drop the original handler table for the callback
                    currentProxy.handlerTable[eventName] = function(origHandlerTable, ...)
                        eventHandler(...)
                    end
                else
                    Debug.Warning(false, string.format("Invalid value passed to multihandler for key %s: %s. Function expected.", eventName, type(eventHandler)))
                end
            end

            -- add a handler keyed on busid.  Store the handler and callback by strong reference
            currentProxy.busHandler = bus.Connect(currentProxy.handlerTable, busId)
        end

        -- Setup disconnect
        function proxyHandler:Disconnect()
            for busId, handlers in pairs(self.handlers) do
                handlers.busHandler:Disconnect()
                handlers.handlerTable = nil
                handlers.origHandlerTable = nil
            end
            self.handlers = nil
        end

        return proxyHandler
    end
end
return ConnectMultiHandlers