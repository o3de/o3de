/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Script/ScriptContext.h>
#include <AzCore/Script/ScriptSystemBus.h>
#include <Execution/Interpreted/ExecutionInterpretedAPI.h>
#include <Execution/Interpreted/ExecutionStateInterpreted.h>
#include <Execution/Interpreted/ExecutionStateInterpretedUtility.h>
#include <Execution/RuntimeComponent.h>

namespace ScriptCanvas
{
    ExecutionStateInterpreted::ExecutionStateInterpreted(ExecutionStateConfig& config)
        : ExecutionState(config)
        , m_interpretedAsset(config.runtimeData.m_script)
    {
        RuntimeAsset* runtimeAsset = config.overrides.m_runtimeAsset.Get();

#if defined(SCRIPT_CANVAS_RUNTIME_ASSET_CHECK)
        if (!runtimeAsset)
        {
            AZ_Error("ScriptCanvas", false
                , "ExecutionStateInterpreted created with ExecutionStateConfig that contained bad runtime asset data. %s"
                , config.overrides.m_runtimeAsset.GetId().ToString<AZStd::string>().data());
            return;
        }
#else
        AZ_Assert(false
            , "ExecutionStateInterpreted created with ExecutionStateConfig that contained bad runtime asset data. %s"
            , config.overrides.m_runtimeAsset.GetId().ToString<AZStd::string>().data());
#endif

        if (!runtimeAsset->m_runtimeData.m_areScriptLocalStaticsInitialized)
        {
            Execution::InitializeInterpretedStatics(runtimeAsset->m_runtimeData);
        }
    }

    void ExecutionStateInterpreted::ClearLuaRegistryIndex()
    {
        m_luaRegistryIndex = LUA_NOREF;
    }

    ExecutionMode ExecutionStateInterpreted::GetExecutionMode() const
    {
        return ExecutionMode::Interpreted;
    }

    int ExecutionStateInterpreted::GetLuaRegistryIndex() const
    {
        return m_luaRegistryIndex;
    }

    lua_State* ExecutionStateInterpreted::LoadLuaScript()
    {
        AZ::ScriptLoadResult result{};
        AZ::ScriptSystemRequestBus::BroadcastResult(result, &AZ::ScriptSystemRequests::LoadAndGetNativeContext, m_interpretedAsset, AZ::k_scriptLoadBinary, AZ::ScriptContextIds::DefaultScriptContextId);
        AZ_Assert(result.status != AZ::ScriptLoadResult::Status::Failed, "ExecutionStateInterpreted script asset was valid but failed to load.");
        AZ_Assert(result.lua, "Must have a default script context and a lua_State");
        AZ_Assert(lua_istable(result.lua, -1), "No run-time execution was available for this script");
        m_luaState = result.lua;
        return result.lua;
    }

    void ExecutionStateInterpreted::ReferenceExecutionState()
    {
        AZ_Assert(m_luaRegistryIndex == LUA_NOREF, "ExecutionStateInterpreted already in the Lua registry and risks double deletion");
        // Lua: instance
        m_luaRegistryIndex = luaL_ref(m_luaState, LUA_REGISTRYINDEX);
        AZ_Assert(m_luaRegistryIndex != LUA_REFNIL, "ExecutionStateInterpreted was nil when trying to gain a reference");
        AZ_Assert(m_luaRegistryIndex != LUA_NOREF, "ExecutionStateInterpreted failed to gain a reference");
    }

    void ExecutionStateInterpreted::ReleaseExecutionState()
    {
        if (m_luaRegistryIndex != LUA_NOREF)
        {
            ReleaseExecutionStateUnchecked();
        }
    }

    void ExecutionStateInterpreted::ReleaseExecutionStateUnchecked()
    {
        luaL_unref(m_luaState, LUA_REGISTRYINDEX, m_luaRegistryIndex);
        m_luaRegistryIndex = LUA_NOREF;
    }
}
