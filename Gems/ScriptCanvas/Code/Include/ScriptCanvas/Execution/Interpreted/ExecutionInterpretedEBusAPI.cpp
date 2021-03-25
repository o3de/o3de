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

#include "ExecutionInterpretedAPI.h"

#include <AzCore/Math/Crc.h>
#include <AzCore/Script/ScriptContext.h>
#include <AzCore/Script/ScriptSystemBus.h>
#include <AzCore/Script/lua/lua.h>

#include <ScriptCanvas/Core/EBusHandler.h>
#include <ScriptCanvas/Core/Nodeable.h>
#include <ScriptCanvas/Core/NodeableOut.h>
#include <ScriptCanvas/Execution/ExecutionState.h>
#include <ScriptCanvas/Execution/Interpreted/ExecutionStateInterpreted.h>
#include <ScriptCanvas/Execution/NodeableOut/NodeableOutNative.h>
#include <ScriptCanvas/Grammar/PrimitivesDeclarations.h>

#include "ExecutionInterpretedOut.h"

namespace ScriptCanvas
{
    namespace Execution
    {
        int EBusHandlerConnect(lua_State* lua)
        {
            auto ebusHandler = AZ::ScriptValue<EBusHandler*>::StackRead(lua, -1);
            AZ_Assert(ebusHandler, "Error in compiled lua file, 1st argument to EBusHandlerConnect is not an EbusHandler");
            lua_pushboolean(lua, ebusHandler->Connect());
            return 1;
        }

        int EBusHandlerConnectTo(lua_State* lua)
        {
            auto ebusHandler = AZ::ScriptValue<EBusHandler*>::StackRead(lua, -3);
            AZ_Assert(ebusHandler, "Error in compiled lua file, 1st argument to EBusHandlerConnectTo is not an EbusHandler");

            const char* aztypeidStr = AZ::ScriptValue<const char*>::StackRead(lua, -2);
            AZ_Assert(aztypeidStr, "Error compiled lua file, 2nd argument to EBusHandlerConnectTo is not a string");

            auto& behaviorContext = *AZ::ScriptContext::FromNativeContext(lua)->GetBoundContext();

            AZ::StackVariableAllocator tempData;
            AZ::BehaviorValueParameter address = BehaviorValueParameterFromTypeIdString(aztypeidStr, behaviorContext);
            AZ_Verify(StackRead(lua, &behaviorContext, -1, address, &tempData), "Error in compiled lua file, failed to read 2nd argument to EBusHandlerConnectTo");

            lua_pushboolean(lua, ebusHandler->ConnectTo(address));
            return 1;
        }

        int EBusHandlerCreate(lua_State* lua)
        {
            // Lua: executionState, (event name) string
            auto executionState = AZ::ScriptValue<ExecutionStateInterpreted*>::StackRead(lua, 1);
            auto ebusName = AZ::ScriptValue<const char*>::StackRead(lua, 2);
            EBusHandler* ebusHandler = aznew EBusHandler(executionState->WeakFromThis(), ebusName, AZ::ScriptContext::FromNativeContext(lua)->GetBoundContext());
            AZ::Internal::LuaClassToStack(lua, ebusHandler, azrtti_typeid<EBusHandler>(), AZ::ObjectToLua::ByReference, AZ::AcquisitionOnPush::ScriptAcquire);
            // Lua: executionState, (event name) string, handler
            return 1;
        }

        int EBusHandlerCreateAndConnect(lua_State* lua)
        {
            // Lua: executionState, (event name) string,
            auto executionState = AZ::ScriptValue<ExecutionStateInterpreted*>::StackRead(lua, 1);
            auto ebusName = AZ::ScriptValue<const char*>::StackRead(lua, 2);
            EBusHandler* ebusHandler = aznew EBusHandler(executionState->WeakFromThis(), ebusName, AZ::ScriptContext::FromNativeContext(lua)->GetBoundContext());
            ebusHandler->Connect();
            AZ::Internal::LuaClassToStack(lua, ebusHandler, azrtti_typeid<EBusHandler>(), AZ::ObjectToLua::ByReference, AZ::AcquisitionOnPush::ScriptAcquire);
            // Lua: executionState, (event name) string, handler
            return 1;
        }

        int EBusHandlerCreateAndConnectTo(lua_State* lua)
        {
            // Lua: executionState, (event name) string, (address aztypeid) string, (address) ?
            auto executionState = AZ::ScriptValue<ExecutionStateInterpreted*>::StackRead(lua, 1);
            auto ebusName = AZ::ScriptValue<const char*>::StackRead(lua, 2);
            EBusHandler* ebusHandler = aznew EBusHandler(executionState->WeakFromThis(), ebusName, AZ::ScriptContext::FromNativeContext(lua)->GetBoundContext());
            const char* aztypeidStr = AZ::ScriptValue<const char*>::StackRead(lua, 3);
            AZ_Assert(aztypeidStr, "Error compiled lua file, 2nd argument to EBusHandlerCreateAndConnectTo is not a string");
            auto& behaviorContext = *AZ::ScriptContext::FromNativeContext(lua)->GetBoundContext();
            AZ::StackVariableAllocator tempData;
            AZ::BehaviorValueParameter address = BehaviorValueParameterFromTypeIdString(aztypeidStr, behaviorContext);
            AZ_Verify(StackRead(lua, &behaviorContext, 4, address, &tempData), "Error in compiled lua file, failed to read 4th argument to EBusHandlerCreateAndConnectTo");
            ebusHandler->ConnectTo(address);

            AZ::Internal::LuaClassToStack(lua, ebusHandler, azrtti_typeid<EBusHandler>(), AZ::ObjectToLua::ByReference, AZ::AcquisitionOnPush::ScriptAcquire);
            // Lua: executionState, (event name) string, (address aztypeid) string, (address) ?, handler
            return 1;
        }

        int EBusHandlerDisconnect(lua_State* lua)
        {
            auto ebusHandler = AZ::ScriptValue<EBusHandler*>::StackRead(lua, -1);
            AZ_Assert(ebusHandler, "Error in compiled lua file, 1st argument to EBusHandlerDisconnect is not an EbusHandler");
            ebusHandler->Disconnect();
            return 0;
        }

        int EBusHandlerHandleEvent(lua_State* lua)
        {
            const int k_nodeableIndex = -3;
            const int k_eventNameIndex = -2;
            const int k_lambdaIndex = -1;

            AZ_Assert(lua_isuserdata(lua, k_nodeableIndex), "Error in compiled lua file, 1st argument to SetExecutionOut is not userdata (Nodeable)");
            AZ_Assert(lua_isstring(lua, k_eventNameIndex), "Error in compiled lua file, 2nd argument to SetExecutionOut is not a string (Crc key)");
            AZ_Assert(lua_isfunction(lua, k_lambdaIndex), "Error in compiled lua file, 3rd argument to SetExecutionOut is not a function (lambda need to get around atypically routed arguments)");

            auto nodeable = AZ::ScriptValue<EBusHandler*>::StackRead(lua, k_nodeableIndex); // this won't be a BCO, because BCOs won't be necessary in the interpreted mode...most likely
            AZ_Assert(nodeable, "Failed to read EBusHandler");
            const char* keyStr = lua_tostring(lua, k_eventNameIndex);
            AZ_Assert(keyStr, "Failed to read key string");
            const int eventIndex = nodeable->GetEventIndex(keyStr);
            AZ_Assert(eventIndex != -1, "Event index was not found for %s-%s", nodeable->GetEBusName().data(), keyStr);
            // install the generic hook for the event
            nodeable->HandleEvent(eventIndex);
            // Lua: nodeable, string, lambda

            lua_pushvalue(lua, k_lambdaIndex);
            // Lua: nodeable, string, lambda, lambda

            // route the event handling to the lambda on the top of the stack
            nodeable->SetExecutionOut(AZ::Crc32(eventIndex), OutInterpreted(lua));
            // Lua: nodeable, string, lambda

            return 0;
        }


        int EBusHandlerHandleEventResult(lua_State* lua)
        {
            const int k_nodeableIndex = -3;
            const int k_eventNameIndex = -2;
            const int k_lambdaIndex = -1;

            AZ_Assert(lua_isuserdata(lua, k_nodeableIndex), "Error in compiled lua file, 1st argument to SetExecutionOut is not userdata (Nodeable)");
            AZ_Assert(lua_isstring(lua, k_eventNameIndex), "Error in compiled lua file, 2nd argument to SetExecutionOut is not a string (Crc key)");
            AZ_Assert(lua_isfunction(lua, k_lambdaIndex), "Error in compiled lua file, 3rd argument to SetExecutionOut is not a function (lambda need to get around atypically routed arguments)");

            auto nodeable = AZ::ScriptValue<EBusHandler*>::StackRead(lua, k_nodeableIndex); // this won't be a BCO, because BCOs won't be necessary in the interpreted mode...most likely
            AZ_Assert(nodeable, "Failed to read EBusHandler");
            const char* keyStr = lua_tostring(lua, k_eventNameIndex);
            AZ_Assert(keyStr, "Failed to read key string");
            const int eventIndex = nodeable->GetEventIndex(keyStr);
            AZ_Assert(eventIndex != -1, "Event index was not found for %s-%s", nodeable->GetEBusName().data(), keyStr);
            // install the generic hook for the event
            nodeable->HandleEvent(eventIndex);
            // Lua: nodeable, string, lambda

            lua_pushvalue(lua, k_lambdaIndex);
            // Lua: nodeable, string, lambda, lambda

            // route the event handling to the lambda on the top of the stack
            nodeable->SetExecutionOut(AZ::Crc32(eventIndex), OutInterpretedResult(lua));
            // Lua: nodeable, string, lambda

            return 0;
        }

        // Lua: (ebus handler) userdata
        int EBusHandlerIsConnected(lua_State* lua)
        {
            auto ebusHandler = AZ::ScriptValue<EBusHandler*>::StackRead(lua, -1);
            AZ_Assert(ebusHandler, "Error in compiled lua file, 1st argument to EBusHandlerIsConnected is not an EbusHandler");
            lua_pushboolean(lua, ebusHandler->IsConnected());
            return 1;
        }

        // Lua: (ebus handler) userdata, (address aztypeid) string, (address) ?
        int EBusHandlerIsConnectedTo(lua_State* lua)
        {
            // Lua: userdata, string, ?
            auto ebusHandler = AZ::ScriptValue<EBusHandler*>::StackRead(lua, -3);
            AZ_Assert(ebusHandler, "Error in compiled lua file, 1st argument to EBusHandlerIsConnectedTo is not an EbusHandler");

            const char* aztypeidStr = lua_tostring(lua, -2);
            AZ_Assert(aztypeidStr, "Error compiled lua file, 2nd argument to EBusHandlerIsConnectedTo is not a string");

            auto& behaviorContext = *AZ::ScriptContext::FromNativeContext(lua)->GetBoundContext();

            AZ::StackVariableAllocator tempData;
            AZ::BehaviorValueParameter address = BehaviorValueParameterFromTypeIdString(aztypeidStr, behaviorContext);
            AZ_Verify(StackRead(lua, &behaviorContext, -1, address, &tempData), "Error in compiled lua file, failed to read 2nd argument to EBusHandlerIsConnectedTo");

            lua_pushboolean(lua, ebusHandler->IsConnectedTo(address));
            // Lua: userdata, string, ?, boolean
            return 1;
        }

        void RegisterEBusHandlerAPI(lua_State* lua)
        {
            using namespace ScriptCanvas::Grammar;

            lua_register(lua, k_EBusHandlerConnectName, &EBusHandlerConnect);
            lua_register(lua, k_EBusHandlerConnectToName, &EBusHandlerConnectTo);
            lua_register(lua, k_EBusHandlerCreateName, &EBusHandlerCreate);
            lua_register(lua, k_EBusHandlerCreateAndConnectName, &EBusHandlerCreateAndConnect);
            lua_register(lua, k_EBusHandlerCreateAndConnectToName, &EBusHandlerCreateAndConnectTo);
            lua_register(lua, k_EBusHandlerDisconnectName, &EBusHandlerDisconnect);
            lua_register(lua, k_EBusHandlerHandleEventName, &EBusHandlerHandleEvent);
            lua_register(lua, k_EBusHandlerHandleEventResultName, &EBusHandlerHandleEventResult);
            lua_register(lua, k_EBusHandlerIsConnectedName, &EBusHandlerIsConnected);
            lua_register(lua, k_EBusHandlerIsConnectedToName, &EBusHandlerIsConnectedTo);
        }
    }

} 
