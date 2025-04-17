/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Asset/AssetManager.h>
#include <ScriptCanvas/Asset/RuntimeAsset.h>
#include <ScriptCanvas/Asset/SubgraphInterfaceAsset.h>
#include <ScriptCanvas/Execution/RuntimeComponent.h>
#include <ScriptCanvas/Execution/ExecutionState.h>

namespace ExecutionStateCpp
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
    ExecutionStateConfig::ExecutionStateConfig(const RuntimeDataOverrides& overrides, ExecutionUserData&& userData)
        : runtimeData(overrides.m_runtimeAsset.Get()->m_runtimeData)
        , overrides(overrides)
        , userData(AZStd::move(userData))
    {}
    
    ExecutionState::ExecutionState(ExecutionStateConfig& config)
        : m_runtimeData(config.runtimeData)
        , m_overrides(config.overrides)
        , m_userData(AZStd::move(config.userData))
    {}

    AZ::Data::AssetId ExecutionState::GetAssetId() const
    {
        return m_overrides.m_runtimeAsset.GetId();
    }

    const Grammar::DebugExecution* ExecutionState::GetDebugSymbolIn(size_t index) const
    {
        return index < m_runtimeData.m_debugMap.m_ins.size()
            ? &(m_runtimeData.m_debugMap.m_ins[index])
            : nullptr;
    }

    const Grammar::DebugExecution* ExecutionState::GetDebugSymbolIn(size_t index, const AZ::Data::AssetId& id) const
    {
        auto asset = ExecutionStateCpp::GetSubgraphAssetForDebug(id);
        return asset && asset.Get() && index < asset.Get()->m_runtimeData.m_debugMap.m_ins.size()
            ? &(asset.Get()->m_runtimeData.m_debugMap.m_ins[index])
            : nullptr;
    }

    const Grammar::DebugExecution* ExecutionState::GetDebugSymbolOut(size_t index) const
    {
        return index < m_runtimeData.m_debugMap.m_outs.size()
            ? &(m_runtimeData.m_debugMap.m_outs[index])
            : nullptr;
    }

    const Grammar::DebugExecution* ExecutionState::GetDebugSymbolOut(size_t index, const AZ::Data::AssetId& id) const
    {
        auto asset = ExecutionStateCpp::GetSubgraphAssetForDebug(id);
        return asset && asset.Get() && index < asset.Get()->m_runtimeData.m_debugMap.m_outs.size()
            ? &(asset.Get()->m_runtimeData.m_debugMap.m_outs[index])
            : nullptr;
    }

    const Grammar::DebugExecution* ExecutionState::GetDebugSymbolReturn(size_t index) const
    {
        return index < m_runtimeData.m_debugMap.m_returns.size()
            ? &(m_runtimeData.m_debugMap.m_returns[index])
            : nullptr;
    }

    const Grammar::DebugExecution* ExecutionState::GetDebugSymbolReturn(size_t index, const AZ::Data::AssetId& id) const
    {
        auto asset = ExecutionStateCpp::GetSubgraphAssetForDebug(id);
        return asset && asset.Get() && index < asset.Get()->m_runtimeData.m_debugMap.m_returns.size()
            ? &(asset.Get()->m_runtimeData.m_debugMap.m_returns[index])
            : nullptr;
    }

    const Grammar::DebugDataSource* ExecutionState::GetDebugSymbolVariableChange(size_t index) const
    {
        return index < m_runtimeData.m_debugMap.m_variables.size()
            ? &(m_runtimeData.m_debugMap.m_variables[index])
            : nullptr;
    }

    const Grammar::DebugDataSource* ExecutionState::GetDebugSymbolVariableChange(size_t index, const AZ::Data::AssetId& id) const
    {
        auto asset = ExecutionStateCpp::GetSubgraphAssetForDebug(id);
        return asset && asset.Get() && index < asset.Get()->m_runtimeData.m_debugMap.m_variables.size()
            ? &(asset.Get()->m_runtimeData.m_debugMap.m_variables[index])
            : nullptr;
    }

    const RuntimeDataOverrides& ExecutionState::GetRuntimeDataOverrides() const
    {
        return m_overrides;
    }

    const RuntimeData& ExecutionState::GetRuntimeData() const
    {
        return m_runtimeData;
    }

    const ExecutionUserData& ExecutionState::GetUserData() const
    {
        return m_userData;
    }

    ExecutionUserData& ExecutionState::ModUserData() const
    {
        return const_cast<ExecutionUserData&>(m_userData);
    }

    ExecutionStatePtr ExecutionState::SharedFromThis()
    {
        return this;
    }

    ExecutionStateConstPtr ExecutionState::SharedFromThisConst() const
    {
        return this;
    }

    AZStd::string ExecutionState::ToString() const
    {
        return AZStd::string::format("ExecutionState[%p]", this);
    }

    ExecutionStateWeakPtr ExecutionState::WeakFromThis()
    {
        return ExecutionStateWeakPtr(this);
    }

    ExecutionStateWeakConstPtr ExecutionState::WeakFromThisConst() const
    {
        return ExecutionStateWeakConstPtr(this);
    }
}
