/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Script/ScriptContext.h>
#include <AzCore/Script/ScriptSystemBus.h>
#include <Execution/Interpreted/ExecutionInterpretedAPI.h>
#include <Execution/Interpreted/ExecutionStateInterpreted.h>
#include <Execution/Interpreted/ExecutionStateInterpretedUtility.h>
#include <Execution/RuntimeComponent.h>

namespace ExecutionStateInterpretedCpp
{
    using namespace ScriptCanvas;

    AZ::Data::Asset<RuntimeAsset> GetSubgraphAssetForDebug(const AZ::Data::AssetId& id)
    {
        // #functions2 this may have to be made recursive
        auto asset = AZ::Data::AssetManager::Instance().GetAsset<SubgraphInterfaceAsset>(id, AZ::Data::AssetLoadBehavior::PreLoad);
        asset.BlockUntilLoadComplete();
        return asset;
    }
}

namespace ScriptCanvas
{
    ExecutionStateInterpreted::ExecutionStateInterpreted(const ExecutionStateConfig& config)
        : ExecutionState(config)
        , m_interpretedAsset(config.runtimeData.m_script)
    {
        RuntimeAsset* runtimeAsset = config.asset.Get();

#if defined(SCRIPT_CANVAS_RUNTIME_ASSET_CHECK)
        if (!runtimeAsset)
        {
            AZ_Error("ScriptCanvas", false
                , "ExecutionStateInterpreted created with ExecutionStateConfig that contained bad runtime asset data. %s"
                , config.asset.GetId().ToString<AZStd::string>().data());
            return;
        }
#else
        AZ_Assert(false
            , "ExecutionStateInterpreted created with ExecutionStateConfig that contained bad runtime asset data. %s"
            , config.asset.GetId().ToString<AZStd::string>().data());
#endif

        Execution::InitializeInterpretedStatics(runtimeAsset->GetData());
    }

    void ExecutionStateInterpreted::ClearLuaRegistryIndex()
    {
        m_luaRegistryIndex = LUA_NOREF;
    }

    const Grammar::DebugExecution* ExecutionStateInterpreted::GetDebugSymbolIn(size_t index) const
    {
        return index < m_component->GetRuntimeAssetData().m_debugMap.m_ins.size()
            ? &(m_component->GetRuntimeAssetData().m_debugMap.m_ins[index])
            : nullptr;
    }

    const Grammar::DebugExecution* ExecutionStateInterpreted::GetDebugSymbolIn(size_t index, const AZ::Data::AssetId& id) const
    {
        auto asset = ExecutionStateInterpretedCpp::GetSubgraphAssetForDebug(id);
        return asset && asset.Get() && index < asset.Get()->GetData().m_debugMap.m_ins.size()
            ? &(asset.Get()->GetData().m_debugMap.m_ins[index])
            : nullptr;
    }

    const Grammar::DebugExecution* ExecutionStateInterpreted::GetDebugSymbolOut(size_t index) const
    {
        return index < m_component->GetRuntimeAssetData().m_debugMap.m_outs.size()
            ? &(m_component->GetRuntimeAssetData().m_debugMap.m_outs[index])
            : nullptr;
    }

    const Grammar::DebugExecution* ExecutionStateInterpreted::GetDebugSymbolOut(size_t index, const AZ::Data::AssetId& id) const
    {
        auto asset = ExecutionStateInterpretedCpp::GetSubgraphAssetForDebug(id);
        return asset && asset.Get() && index < asset.Get()->GetData().m_debugMap.m_outs.size()
            ? &(asset.Get()->GetData().m_debugMap.m_outs[index])
            : nullptr;
    }

    const Grammar::DebugExecution* ExecutionStateInterpreted::GetDebugSymbolReturn(size_t index) const
    {
        return index < m_component->GetRuntimeAssetData().m_debugMap.m_returns.size()
            ? &(m_component->GetRuntimeAssetData().m_debugMap.m_returns[index])
            : nullptr;
    }

    const Grammar::DebugExecution* ExecutionStateInterpreted::GetDebugSymbolReturn(size_t index, const AZ::Data::AssetId& id) const
    {
        auto asset = ExecutionStateInterpretedCpp::GetSubgraphAssetForDebug(id);
        return asset && asset.Get() && index < asset.Get()->GetData().m_debugMap.m_returns.size()
            ? &(asset.Get()->GetData().m_debugMap.m_returns[index])
            : nullptr;
    }

    const Grammar::DebugDataSource* ExecutionStateInterpreted::GetDebugSymbolVariableChange(size_t index) const
    {
        return index < m_component->GetRuntimeAssetData().m_debugMap.m_variables.size()
            ? &(m_component->GetRuntimeAssetData().m_debugMap.m_variables[index])
            : nullptr;
    }

    const Grammar::DebugDataSource* ExecutionStateInterpreted::GetDebugSymbolVariableChange(size_t index, const AZ::Data::AssetId& id) const
    {
        auto asset = ExecutionStateInterpretedCpp::GetSubgraphAssetForDebug(id);
        return asset && asset.Get() && index < asset.Get()->GetData().m_debugMap.m_variables.size()
            ? &(asset.Get()->GetData().m_debugMap.m_variables[index])
            : nullptr;
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
    }

    void ExecutionStateInterpreted::Reflect(AZ::ReflectContext* reflectContext)
    {
        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(reflectContext))
        {
            behaviorContext->Class<ExecutionStateInterpreted>()
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::List)
                ->Attribute(AZ::ScriptCanvasAttributes::VariableCreationForbidden, AZ::AttributeIsValid::IfPresent)
                ;
        }
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
