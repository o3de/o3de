/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ExecutionInterpretedOut.h"

#include <AzCore/Math/Crc.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Script/ScriptContext.h>
#include <AzCore/Script/ScriptSystemBus.h>
#include <AzCore/Script/lua/lua.h>
#include <ScriptCanvas/Core/EBusHandler.h>
#include <ScriptCanvas/Core/Nodeable.h>
#include <ScriptCanvas/Core/NodeableOut.h>
#include <ScriptCanvas/Execution/ExecutionState.h>
#include <ScriptCanvas/Execution/NodeableOut/NodeableOutNative.h>
#include <ScriptCanvas/Grammar/PrimitivesDeclarations.h>

#include "ExecutionInterpretedAPI.h"

namespace ExecutionStateInterpretedCpp
{
    AZ_FORCE_INLINE int luaL_ref_Checked(lua_State* lua)
    {
        AZ_Assert(lua, "null lua_State");
        int ref = luaL_ref(lua, LUA_REGISTRYINDEX);
        AZ_Assert(ref != LUA_NOREF && ref != LUA_REFNIL, "invalid lambdaRegistryIndex");
        return ref;
    }
}

namespace ScriptCanvas
{
    namespace Execution
    {        
        OutInterpreted::OutInterpreted(lua_State* lua)
            : m_lambdaRegistryIndex(ExecutionStateInterpretedCpp::luaL_ref_Checked(lua))
            , m_lua(lua)
        {
        }

        OutInterpreted::OutInterpreted(OutInterpreted&& source) noexcept
        {
            *this = AZStd::move(source);
        }

        OutInterpreted::~OutInterpreted()
        {
            luaL_unref(m_lua, LUA_REGISTRYINDEX, m_lambdaRegistryIndex);
        }

        OutInterpreted::OutInterpreted(const OutInterpreted& /*source*/)
        {
            AZ_Assert(false, "This will result in a double delete of a Lua function. Don't use this");
        }

        OutInterpreted& OutInterpreted::operator=(OutInterpreted&& source) noexcept
        {
            AZ_Assert(this != &source, "abuse of OutInterpreted move operator");
            m_lambdaRegistryIndex = source.m_lambdaRegistryIndex;
            source.m_lambdaRegistryIndex = LUA_NOREF;
            m_lua = source.m_lua;
            source.m_lua = nullptr;
            return *this;
        }

        void OutInterpreted::operator()(AZ::BehaviorValueParameter* /*resultBVP*/, AZ::BehaviorValueParameter* argsBVPs, int numArguments)
        {
            auto behaviorContext = AZ::ScriptContext::FromNativeContext(m_lua)->GetBoundContext();
            // Lua:
            lua_rawgeti(m_lua, LUA_REGISTRYINDEX, m_lambdaRegistryIndex);
            // Lua: lambda

            for (int i = 0; i < numArguments; ++i)
            {
                Execution::StackPush(m_lua, behaviorContext, argsBVPs[i]);
            }
            // Lua: lambda, args...
            const int result = InterpretedSafeCall(m_lua, numArguments, 0);
            // Lua: ?
            if (result != LUA_OK)
            {
                // Lua: error
                lua_pop(m_lua, 1);
            }
            // Lua:
        }

        OutInterpretedResult::OutInterpretedResult(lua_State* lua)
            : m_lambdaRegistryIndex(ExecutionStateInterpretedCpp::luaL_ref_Checked(lua))
            , m_lua(lua)
        {
        }

        OutInterpretedResult::OutInterpretedResult(OutInterpretedResult&& source) noexcept
        {
            *this = AZStd::move(source);
        }

        OutInterpretedResult::~OutInterpretedResult()
        {
            luaL_unref(m_lua, LUA_REGISTRYINDEX, m_lambdaRegistryIndex);
        }

        OutInterpretedResult::OutInterpretedResult(const OutInterpretedResult& /*source*/)
        {
            AZ_Assert(false, "This will result in a double delete of a Lua function. Don't use this");
        }

        OutInterpretedResult& OutInterpretedResult::operator=(OutInterpretedResult&& source) noexcept
        {
            AZ_Assert(this != &source, "abuse of OutInterpretedResult move operator");
            m_lambdaRegistryIndex = source.m_lambdaRegistryIndex;
            source.m_lambdaRegistryIndex = LUA_NOREF;
            m_lua = source.m_lua;
            source.m_lua = nullptr;
            return *this;
        }

        void OutInterpretedResult::operator()(AZ::BehaviorValueParameter* resultBVP, AZ::BehaviorValueParameter* argsBVPs, int numArguments)
        {
            AZ_Assert(resultBVP && resultBVP->m_value, "This function is only expected for BehaviorConext bound event handling, and will always have a location for a return value");

            auto behaviorContext = AZ::ScriptContext::FromNativeContext(m_lua)->GetBoundContext();
            // Lua:
            lua_rawgeti(m_lua, LUA_REGISTRYINDEX, m_lambdaRegistryIndex);
            // Lua: lambda

            for (int i = 0; i < numArguments; ++i)
            {
                Execution::StackPush(m_lua, behaviorContext, argsBVPs[i]);
            }
            // Lua: lambda, args...
            const int result = InterpretedSafeCall(m_lua, numArguments, 1);
            // Lua: ?
            if (result != LUA_OK)
            {
                // Lua: error
                lua_pop(m_lua, 1);
            }
            else
            {
                // Lua: result
                Execution::StackRead(m_lua, behaviorContext, -1, *resultBVP, nullptr);
                lua_pop(m_lua, 1);
            }
            // Lua:
        }

        OutInterpretedUserSubgraph::OutInterpretedUserSubgraph(lua_State* lua)
            : m_lambdaRegistryIndex(ExecutionStateInterpretedCpp::luaL_ref_Checked(lua))
            , m_lua(lua)
        {
        }

        OutInterpretedUserSubgraph::OutInterpretedUserSubgraph(OutInterpretedUserSubgraph&& source) noexcept
        {
            *this = AZStd::move(source);
        }

        OutInterpretedUserSubgraph::~OutInterpretedUserSubgraph()
        {
            luaL_unref(m_lua, LUA_REGISTRYINDEX, m_lambdaRegistryIndex);
        }

        OutInterpretedUserSubgraph::OutInterpretedUserSubgraph(const OutInterpretedUserSubgraph& /*source*/)
        {
            AZ_Assert(false, "This will result in a double delete of a Lua function. Don't use this");
        }

        OutInterpretedUserSubgraph& OutInterpretedUserSubgraph::operator=(OutInterpretedUserSubgraph&& source) noexcept
        {
            AZ_Assert(this != &source, "abuse of OutInterpretedUserSubgraph move operator");
            m_lambdaRegistryIndex = source.m_lambdaRegistryIndex;
            source.m_lambdaRegistryIndex = LUA_NOREF;
            m_lua = source.m_lua;
            source.m_lua = nullptr;
            return *this;
        }

        void OutInterpretedUserSubgraph::operator()(AZ::BehaviorValueParameter* /*resultBVP*/, AZ::BehaviorValueParameter* /*argsBVPs*/, int argsCount)
        {
            // Lua: executionState, outKey, args...
            /*auto behaviorContext =*/ AZ::ScriptContext::FromNativeContext(m_lua)->GetBoundContext();
            lua_rawgeti(m_lua, LUA_REGISTRYINDEX, m_lambdaRegistryIndex);
            // Lua: executionState, outKey, args..., lambda
            lua_remove(m_lua, 1);
            // Lua: outKey, args..., lambda
            lua_replace(m_lua, 1);
            AZ_Assert(lua_isfunction(m_lua, 1), "OutInterpretedUserSubgraph::operator(): Error in compiled lua file, user lambda was not found");
            // Lua: lambda, args...
            const int result = InterpretedSafeCall(m_lua, argsCount, LUA_MULTRET);
            // Lua: ?
            if (result != LUA_OK)
            {
                // Lua: error
                lua_pop(m_lua, 1);
            }
            else
            {
                // Lua: results...
            }
        }
    }
}
