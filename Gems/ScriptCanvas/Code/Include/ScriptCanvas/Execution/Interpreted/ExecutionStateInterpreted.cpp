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

#if defined(SC_RUNTIME_CHECKS_ENABLED)
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

    ExecutionMode ExecutionStateInterpreted::GetExecutionMode() const
    {
        return ExecutionMode::Interpreted;
    }

    lua_State* ExecutionStateInterpreted::LoadLuaScript()
    {
        AZ::ScriptLoadResult result{};
        AZ::ScriptSystemRequestBus::BroadcastResult
            ( result
            , &AZ::ScriptSystemRequests::LoadAndGetNativeContext
            , m_interpretedAsset
            , AZ::k_scriptLoadBinary
            , AZ::ScriptContextIds::DefaultScriptContextId);
        SC_RUNTIME_CHECK(result.status != AZ::ScriptLoadResult::Status::Failed, "ExecutionStateInterpreted script asset failed to load.");
        SC_RUNTIME_CHECK(result.lua, "Must have a default script context and a lua_State");
        SC_RUNTIME_CHECK(lua_istable(result.lua, -1), "No run-time execution was available for this script");
        m_luaState = result.lua;
        return result.lua;
    }

}
