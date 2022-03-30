/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Asset/AssetManager.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <ScriptCanvas/Core/Core.h>
#include <ScriptCanvas/Execution/RuntimeComponent.h>

#include "Interpreted/ExecutionStateInterpreted.h"
#include "Interpreted/ExecutionStateInterpretedPure.h"
#include "Interpreted/ExecutionStateInterpretedPerActivation.h"
#include "Interpreted/ExecutionStateInterpretedSingleton.h"

#include "ExecutionState.h"


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
    ExecutionStateConfig::ExecutionStateConfig(const RuntimeDataOverrides& overrides, AZStd::any&& userData)
        : runtimeData(overrides.m_runtimeAsset.Get()->m_runtimeData)
        , overrides(overrides)
        , userData(AZStd::move(userData))
    {}
    
    ExecutionState::ExecutionState(ExecutionStateConfig& config)
        : m_runtimeData(config.runtimeData)
        , m_overrides(config.overrides)
        , m_userData(AZStd::move(config.userData))
    {}

    ExecutionStatePtr ExecutionState::Create(ExecutionStateConfig& config)
    {
        Grammar::ExecutionStateSelection selection = config.runtimeData.m_input.m_executionSelection;

        switch (selection)
        {
        case Grammar::ExecutionStateSelection::InterpretedPure:
            return AZStd::make_shared<ExecutionStateInterpretedPure>(config);

        case Grammar::ExecutionStateSelection::InterpretedPureOnGraphStart:
            return AZStd::make_shared<ExecutionStateInterpretedPureOnGraphStart>(config);

        case Grammar::ExecutionStateSelection::InterpretedObject:
            return AZStd::make_shared<ExecutionStateInterpretedPerActivation>(config);

        case Grammar::ExecutionStateSelection::InterpretedObjectOnGraphStart:
            return AZStd::make_shared<ExecutionStateInterpretedPerActivationOnGraphStart>(config);

        default:
            AZ_Assert(false, "Unsupported ScriptCanvas execution selection");
            return nullptr;
        }
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

    const AZStd::any& ExecutionState::GetUserData() const
    {
        return m_userData;
    }

    AZStd::any& ExecutionState::ModUserData() const
    {
        return const_cast<AZStd::any&>(m_userData);
    }

    void ExecutionState::Reflect(AZ::ReflectContext* reflectContext)
    {
        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(reflectContext))
        {
            behaviorContext->Class<ExecutionState>()
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::List)
                ->Attribute(AZ::ScriptCanvasAttributes::VariableCreationForbidden, AZ::AttributeIsValid::IfPresent)
                ->Method("ToString", &ExecutionState::ToString)
                    ->Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::ToString)
                ;
        }

        ExecutionStateInterpreted::Reflect(reflectContext);
        ExecutionStateInterpretedPerActivation::Reflect(reflectContext);
        ExecutionStateInterpretedPerActivationOnGraphStart::Reflect(reflectContext);
        ExecutionStateInterpretedPure::Reflect(reflectContext);
        ExecutionStateInterpretedPureOnGraphStart::Reflect(reflectContext);
        ExecutionStateInterpretedSingleton::Reflect(reflectContext);
    }

    ExecutionStatePtr ExecutionState::SharedFromThis()
    {
        return shared_from_this();
    }

    ExecutionStateConstPtr ExecutionState::SharedFromThisConst() const
    {
        return shared_from_this();
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
