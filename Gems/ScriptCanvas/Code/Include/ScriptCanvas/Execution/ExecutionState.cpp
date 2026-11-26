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

    const RuntimeAsset* FindSubgraphAssetRecursive(const RuntimeData& runtimeData, const AZ::Data::AssetId& id)
    {
        for (const auto& requiredAsset : runtimeData.m_requiredAssets)
        {
            if (requiredAsset.GetId() == id && requiredAsset.Get())
            {
                return requiredAsset.Get();
            }
            if (requiredAsset.Get())
            {
                if (const RuntimeAsset* found = FindSubgraphAssetRecursive(requiredAsset.Get()->m_runtimeData, id))
                {
                    return found;
                }
            }
        }
        return nullptr;
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

    const RuntimeAsset* ExecutionState::FindSubgraphAsset(const AZ::Data::AssetId& id) const
    {
        if (m_overrides.m_runtimeAsset.GetId() == id && m_overrides.m_runtimeAsset.Get())
        {
            return m_overrides.m_runtimeAsset.Get();
        }
        return ExecutionStateCpp::FindSubgraphAssetRecursive(m_runtimeData, id);
    }

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
        const RuntimeAsset* asset = FindSubgraphAsset(id);
        return asset && index < asset->m_runtimeData.m_debugMap.m_ins.size()
            ? &(asset->m_runtimeData.m_debugMap.m_ins[index])
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
        const RuntimeAsset* asset = FindSubgraphAsset(id);
        return asset && index < asset->m_runtimeData.m_debugMap.m_outs.size()
            ? &(asset->m_runtimeData.m_debugMap.m_outs[index])
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
        const RuntimeAsset* asset = FindSubgraphAsset(id);
        return asset && index < asset->m_runtimeData.m_debugMap.m_returns.size()
            ? &(asset->m_runtimeData.m_debugMap.m_returns[index])
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
        const RuntimeAsset* asset = FindSubgraphAsset(id);
        return asset && index < asset->m_runtimeData.m_debugMap.m_variables.size()
            ? &(asset->m_runtimeData.m_debugMap.m_variables[index])
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
