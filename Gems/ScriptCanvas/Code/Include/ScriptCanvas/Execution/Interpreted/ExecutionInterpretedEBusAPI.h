/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#pragma once

#include <AzCore/std/allocator.h>
#include <AzCore/std/allocator_static.h>
#include <AzCore/std/typetraits/internal/type_sequence_traits.h>

#include <ScriptCanvas/Core/NodeableOut.h>

struct lua_State;

namespace AZ
{
    class SerializeContext;
}

namespace ScriptCanvas
{
    namespace Execution
    {
        // Lua: (ebus handler) userdata
        int EBusHandlerConnect(lua_State* lua);

        // Lua: (ebus handler) userdata, (address aztypeid) string, (address) ?
        int EBusHandlerConnectTo(lua_State* lua);

        // Lua: executionState, (event name) string
        int EBusHandlerCreate(lua_State* lua);

        // Lua: executionState, (event name) string
        int EBusHandlerCreateAndConnect(lua_State* lua);

        // Lua: executionState, (event name) string, (address aztypeid) string, (address) ?
        int EBusHandlerCreateAndConnectTo(lua_State* lua);

        // Lua: (ebus handler) userdata
        int EBusHandlerDisconnect(lua_State* lua);

        // Lua: (ebus handler) userdata, (event name) string, (event implementation) function
        int EBusHandlerHandleEvent(lua_State* lua);

        // Lua: (ebus handler) userdata, (event name) string, (event implementation) function
        int EBusHandlerHandleEventResult(lua_State* lua);

        // Lua: (ebus handler) userdata
        int EBusHandlerIsConnected(lua_State* lua);

        // Lua: (ebus handler) userdata, (address aztypeid) string, (address) ?
        int EBusHandlerIsConnectedTo(lua_State* lua);

        void RegisterEBusHandlerAPI(lua_State* lua);       

    } 

} 
