/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ExecutionInterpretedDebugAPI.h"

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Script/ScriptContext.h>
#include <AzCore/Script/ScriptSystemBus.h>
#include <AzCore/Script/lua/lua.h>
#include <ScriptCanvas/Core/ExecutionNotificationsBus.h>
#include <ScriptCanvas/Grammar/DebugMap.h>
#include <ScriptCanvas/Execution/ExecutionState.h>
#include <ScriptCanvas/Execution/Interpreted/ExecutionStateInterpreted.h>
#include <ScriptCanvas/Execution/Interpreted/ExecutionStateInterpretedAPI.h>

#include <Libraries/UnitTesting/UnitTestBus.h>

namespace ExecutionInterpretedDebugAPIcpp
{
    using namespace ScriptCanvas;
    using namespace ScriptCanvas::Execution;

    void PopulateSignalDatum(lua_State* lua, int stackIndex, DatumValue& datumValue, const ScriptCanvas::Grammar::DebugDataSource* debugDatumSource)
    {
        if (debugDatumSource->m_fromStack)
        {
            Datum datum(debugDatumSource->m_slotDatumType, Datum::eOriginality::Copy);
            AZ::BehaviorClass* behaviorClass(nullptr);
            AZ::BehaviorArgument bvp = datum.ToBehaviorContext(behaviorClass);
            debugDatumSource->m_fromStack(lua, stackIndex, bvp, behaviorClass, nullptr);
            datumValue = DatumValue::Create(datum);
        }
    }

    void PopulateSignalData(lua_State* lua, int stackIndex, Signal& signal, const AZStd::vector<ScriptCanvas::Grammar::DebugDataSource>& debugDataSource)
    {
        for (const auto& debugDatumSource : debugDataSource)
        {
            PopulateSignalDatum(lua, stackIndex, signal.m_data[NamedSlotId(debugDatumSource.m_slotId)], &debugDatumSource);
            // internal debug data will not be stacked up in lua script, skip
            if (debugDatumSource.m_sourceType != ScriptCanvas::Grammar::DebugDataSourceType::Internal
            || debugDatumSource.m_slotDatumType.IsValid())
            {
                ++stackIndex;
            }
        }
    }
}

namespace ScriptCanvas
{
    namespace Execution
    {
        int DebugIsTraced(lua_State* lua)
        {
            const auto executionState = ExecutionStateRead(lua, 1);
            AZ_Assert(executionState, "Error in compiled lua file, 1st argument to DebugIsTraced is not an ExecutionStateInterpreted");
            if (executionState)
            {
                bool isObserved{};
                const auto info = GraphInfo(executionState);
                ExecutionNotificationsBus::BroadcastResult(isObserved, &ExecutionNotifications::IsGraphObserved, info.m_runtimeEntity, info.m_graphIdentifier);
                lua_pushboolean(lua, isObserved);
            }
            else
            {
                lua_pushboolean(lua, false);
            }

            return 1;
        }

        int DebugRuntimeError(lua_State* lua)
        {
            // Lua: executionState, string
            const auto executionState = ExecutionStateRead(lua, -2);
            AZ_Assert(executionState, "Error in compiled lua file, 1st argument to DebugRuntimeError is not an ExecutionStateInterpreted");

            if (executionState)
            {
                // reconfigure this bus to only take the  execution state
                ExecutionNotificationsBus::Broadcast(&ExecutionNotifications::RuntimeError, *executionState, lua_tostring(lua, -1));
            }

            lua_remove(lua, -2);
            lua_error(lua);
            return 1;
        }

        int DebugSignalIn(lua_State* lua)
        {
            const auto executionState = ExecutionStateRead(lua, 1);
            AZ_Assert(executionState, "Error in compiled lua file, 1st argument to DebugSignalIn is not an ExecutionStateInterpreted");
            const size_t debugExecutionIndex = AZ::ScriptValue<size_t>::StackRead(lua, 2);

            if (const Grammar::DebugExecution* debugIn = executionState->GetDebugSymbolIn(debugExecutionIndex))
            {
                auto inSignal = InputSignal(GraphInfo(executionState));
                inSignal.m_endpoint = debugIn->m_namedEndpoint;
                ExecutionInterpretedDebugAPIcpp::PopulateSignalData(lua, 3, inSignal, debugIn->m_data);
                ExecutionNotificationsBus::Broadcast(&ExecutionNotifications::NodeSignaledInput, inSignal);
            }
            else
            {
                AZ_Warning("ScriptCanvas", false, "Missing debug information in DebugSignalIn");
            }

            return 0;
        }

        int DebugSignalInSubgraph(lua_State* lua)
        {
            const auto executionState = ExecutionStateRead(lua, 1);
            AZ_Assert(executionState, "Error in compiled lua file, 1st argument to DebugSignalInSubgraph is not an ExecutionStateInterpreted");
            AZ_Assert(lua_isstring(lua, 2), "Error in compiled Lua file, 2nd argument to DebugSignalInSubgraph is not a string.");
            const char* assetIdString = lua_tostring(lua, 2);
            const AZ::Data::AssetId subgraphId = AZ::Data::AssetId::CreateString(assetIdString);
            const size_t debugExecutionIndex = AZ::ScriptValue<size_t>::StackRead(lua, 3);

            if (const Grammar::DebugExecution* debugIn = executionState->GetDebugSymbolIn(debugExecutionIndex, subgraphId))
            {
                auto inSignal = InputSignal(GraphInfo(executionState));
                inSignal.m_endpoint = debugIn->m_namedEndpoint;
                ExecutionInterpretedDebugAPIcpp::PopulateSignalData(lua, 4, inSignal, debugIn->m_data);
                ExecutionNotificationsBus::Broadcast(&ExecutionNotifications::NodeSignaledInput, inSignal);
            }
            else
            {
                AZ_Warning("ScriptCanvas", false, "Missing debug information in DebugSignalInSubgraph");
            }

            return 0;
        }

        int DebugSignalOut(lua_State* lua)
        {
            const auto executionState = ExecutionStateRead(lua, 1);
            AZ_Assert(executionState, "Error in compiled lua file, 1st argument to DebugSignalOut is not an ExecutionStateInterpreted");
            const size_t debugExecutionIndex = AZ::ScriptValue<size_t>::StackRead(lua, 2);

            if (const Grammar::DebugExecution* debugOut = executionState->GetDebugSymbolOut(debugExecutionIndex))
            {
                auto outSignal = OutputSignal(GraphInfo(executionState));
                outSignal.m_endpoint = debugOut->m_namedEndpoint;
                ExecutionInterpretedDebugAPIcpp::PopulateSignalData(lua, 3, outSignal, debugOut->m_data);
                ExecutionNotificationsBus::Broadcast(&ExecutionNotifications::NodeSignaledOutput, outSignal);
            }
            else
            {
                AZ_Warning("ScriptCanvas", false, "Missing debug information in DebugSignalOut");
            }

            return 0;
        }

        int DebugSignalOutSubgraph(lua_State* lua)
        {
            const auto executionState = ExecutionStateRead(lua, 1);
            AZ_Assert(executionState, "Error in compiled lua file, 1st argument to DebugSignalOutSubgraph is not an ExecutionStateInterpreted");
            AZ_Assert(lua_isstring(lua, 2), "Error in compiled Lua file, 2nd argument to DebugSignalOutSubgraph is not a string.");
            const char* assetIdString = lua_tostring(lua, 2);
            const AZ::Data::AssetId subgraphId = AZ::Data::AssetId::CreateString(assetIdString);
            const size_t debugExecutionIndex = AZ::ScriptValue<size_t>::StackRead(lua, 3);

            if (const Grammar::DebugExecution* debugOut = executionState->GetDebugSymbolOut(debugExecutionIndex, subgraphId))
            {
                auto outSignal = OutputSignal(GraphInfo(executionState));
                outSignal.m_endpoint = debugOut->m_namedEndpoint;
                ExecutionInterpretedDebugAPIcpp::PopulateSignalData(lua, 4, outSignal, debugOut->m_data);
                ExecutionNotificationsBus::Broadcast(&ExecutionNotifications::NodeSignaledOutput, outSignal);
            }
            else
            {
                AZ_Warning("ScriptCanvas", false, "Missing debug informatoin in DebugSignalOutSubgraph");
            }

            return 0;
        }

        int DebugSignalReturn(lua_State* lua)
        {
            const auto executionState = ExecutionStateRead(lua, 1);
            AZ_Assert(executionState, "Error in compiled lua file, 1st argument to DebugSignalReturn is not an ExecutionStateInterpreted");
            const size_t debugExecutionIndex = AZ::ScriptValue<size_t>::StackRead(lua, 2);
            const Grammar::DebugExecution* debugReturn = executionState->GetDebugSymbolReturn(debugExecutionIndex);
            auto returnSignal = ReturnSignal(GraphInfo(executionState));
            returnSignal.m_endpoint = debugReturn->m_namedEndpoint;
            ExecutionInterpretedDebugAPIcpp::PopulateSignalData(lua, 3, returnSignal, debugReturn->m_data);
            ExecutionNotificationsBus::Broadcast(&ExecutionNotifications::GraphSignaledReturn, returnSignal);
            return 0;
        }

        int DebugSignalReturnSubgraph(lua_State* lua)
        {
            const auto executionState = ExecutionStateRead(lua, 1);
            AZ_Assert(executionState, "Error in compiled lua file, 1st argument to DebugSignalReturnSubgraph is not an ExecutionStateInterpreted");
            AZ_Assert(lua_isstring(lua, 2), "Error in compiled Lua file, 2nd argument to DebugSignalReturnSubgraph is not a string.");
            // Todo subgraph id will be needed in the signal data when handling this event
            // const char* assetIdString = lua_tostring(lua, 2);
            // AZ::Data::AssetId subgraphId = AZ::Data::AssetId::CreateString(assetIdString);
            const size_t debugExecutionIndex = AZ::ScriptValue<size_t>::StackRead(lua, 3);

            const Grammar::DebugExecution* debugReturn = executionState->GetDebugSymbolReturn(debugExecutionIndex);
            auto returnSignal = ReturnSignal(GraphInfo(executionState));
            returnSignal.m_endpoint = debugReturn->m_namedEndpoint;
            ExecutionInterpretedDebugAPIcpp::PopulateSignalData(lua, 4, returnSignal, debugReturn->m_data);
            ExecutionNotificationsBus::Broadcast(&ExecutionNotifications::GraphSignaledReturn, returnSignal);

            return 0;
        }

        int DebugVariableChange(lua_State* lua)
        {
            const auto executionState = ExecutionStateRead(lua, 1);
            AZ_Assert(executionState, "Error in compiled lua file, 1st argument to DebugVariableChange is not an ExecutionStateInterpreted");
            const size_t debugVariableChangeIndex = AZ::ScriptValue<size_t>::StackRead(lua, 2);

            if (const Grammar::DebugDataSource* variableChangeSymbol = executionState->GetDebugSymbolVariableChange(debugVariableChangeIndex))
            {
                DatumValue value;
                ExecutionInterpretedDebugAPIcpp::PopulateSignalDatum(lua, 3, value, variableChangeSymbol);
                VariableChange variableChangeSignal(GraphInfo(executionState), value);
                // \todo this signal is missing the variable id
                ExecutionNotificationsBus::Broadcast(&ExecutionNotifications::VariableChanged, variableChangeSignal);
            }
            else
            {
                AZ_Warning("ScriptCanvas", false, "Missing debug information in DebugVariableChange");
            }

            return 0;
        }
        
        int DebugVariableChangeSubgraph(lua_State* lua)
        {
            const auto executionState = ExecutionStateRead(lua, 1);
            AZ_Assert(executionState, "Error in compiled lua file, 1st argument to DebugVariableChangeSubgraph is not an ExecutionStateInterpreted");
            AZ_Assert(lua_isstring(lua, 2), "Error in compiled Lua file, 2nd argument to DebugVariableChangeSubgraph is not a string.");
            const char* assetIdString = lua_tostring(lua, 2);
            const AZ::Data::AssetId subgraphId = AZ::Data::AssetId::CreateString(assetIdString);
            const size_t debugVariableChangeIndex = AZ::ScriptValue<size_t>::StackRead(lua, 3);

            if (const Grammar::DebugDataSource* variableChangeSymbol = executionState->GetDebugSymbolVariableChange(debugVariableChangeIndex, subgraphId))
            {
                DatumValue value;
                ExecutionInterpretedDebugAPIcpp::PopulateSignalDatum(lua, 4, value, variableChangeSymbol);
                VariableChange variableChangeSignal(GraphInfo(executionState), value);
                // \todo this signal is missing the variable id
                ExecutionNotificationsBus::Broadcast(&ExecutionNotifications::VariableChanged, variableChangeSignal);
            }
            else
            {
                AZ_Warning("ScriptCanvas", false, "Missing debug information in DebugVariableChangeSubgraph");
            }

            return 0;
        }

        void RegisterDebugAPI(lua_State* lua)
        {
            using namespace ScriptCanvas::Grammar;

            lua_register(lua, k_DebugIsTracedName, &DebugIsTraced);
            lua_register(lua, k_DebugRuntimeErrorName, &DebugRuntimeError);
            lua_register(lua, k_DebugSignalInName, &DebugSignalIn);
            lua_register(lua, k_DebugSignalInSubgraphName, &DebugSignalInSubgraph);
            lua_register(lua, k_DebugSignalOutName, &DebugSignalOut);
            lua_register(lua, k_DebugSignalOutSubgraphName, &DebugSignalOutSubgraph);
            lua_register(lua, k_DebugSignalReturnName, &DebugSignalReturn);
            lua_register(lua, k_DebugSignalReturnSubgraphName, &DebugSignalReturnSubgraph);
            lua_register(lua, k_DebugVariableChangeName, &DebugVariableChange);
            lua_register(lua, k_DebugVariableChangeSubgraphName, &DebugVariableChangeSubgraph);
        }
        
    } 

} 
