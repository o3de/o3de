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

        void OutInterpreted::operator()(AZ::BehaviorArgument* /*resultBVP*/, AZ::BehaviorArgument* argsBVPs, int numArguments)
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
            AZ_Assert(false, "OutInterpretedResult::OutInterpretedResult copy ctr is for compiler compatibility only.");
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

        void OutInterpretedResult::operator()(AZ::BehaviorArgument* resultBVP, AZ::BehaviorArgument* argsBVPs, int numArguments)
        {
            SC_RUNTIME_CHECK(resultBVP && resultBVP->m_value, "This function is only expected for BehaviorConext bound event handling, and must always have a location for a return value");

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

            if(result == LUA_OK && resultBVP)
            {
                Execution::StackRead(m_lua, behaviorContext, -1, *resultBVP, nullptr);
            }

            lua_pop(m_lua, 1);
        }
    }
}
