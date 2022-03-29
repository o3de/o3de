/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/RTTI/BehaviorContext.h>
#include <ScriptCanvas/Core/Core.h>
#include <ScriptCanvas/Execution/RuntimeComponent.h>

#include "Interpreted/ExecutionStateInterpreted.h"
#include "Interpreted/ExecutionStateInterpretedPure.h"
#include "Interpreted/ExecutionStateInterpretedPerActivation.h"
#include "Interpreted/ExecutionStateInterpretedSingleton.h"

#include "ExecutionState.h"

namespace ScriptCanvas
{
    ExecutionStateConfig::ExecutionStateConfig(AZ::Data::Asset<RuntimeAsset> asset)
        : runtimeData(asset.Get()->m_runtimeData)
        , asset(asset)
    {}

    ExecutionStateConfig::ExecutionStateConfig(AZ::Data::Asset<RuntimeAsset> asset, AZStd::any&& userData)
        : runtimeData(asset.Get()->m_runtimeData)
        , asset(asset)
        , userData(userData)
    {}
    
    ExecutionState::ExecutionState(ExecutionStateConfig& config)
        : m_userData(AZStd::move(config.userData))
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

    // chopping block - begin

    const RuntimeComponent* ExecutionState::GetRuntimeComponent() const
    {
        auto reference = AZStd::any_cast<Execution::Reference>(&m_userData);
        return reference ? reference->As<RuntimeComponent>() : nullptr;
    }

    AZ::Data::AssetId ExecutionState::GetAssetId() const
    {
        return GetRuntimeComponent()->GetRuntimeDataOverrides().m_runtimeAsset.GetId();
    }

    AZ::EntityId ExecutionState::GetEntityId() const
    {
        return GetRuntimeComponent()->GetEntityId();
    }

    AZ::ComponentId ExecutionState::GetComponentId() const
    {
        return GetRuntimeComponent()->GetId();
    }

    GraphIdentifier ExecutionState::GetGraphIdentifier() const
    {
        return GetRuntimeComponent()->GetGraphIdentifier();
    }

    GraphIdentifier ExecutionState::GetGraphIdentifier(const AZ::Data::AssetId& id) const
    {
        return GraphIdentifier(AZ::Data::AssetId(id.m_guid, 0), GetComponentId());
    }

    AZ::EntityId ExecutionState::GetScriptCanvasId() const
    {
        return GetRuntimeComponent()->GetScriptCanvasId();
    }

    const RuntimeDataOverrides& ExecutionState::GetRuntimeDataOverrides() const
    {
        return GetRuntimeComponent()->GetRuntimeDataOverrides();
    }
    // chopping block - end

    void ExecutionState::Reflect(AZ::ReflectContext* reflectContext)
    {
        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(reflectContext))
        {
            behaviorContext->Class<ExecutionState>()
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::List)
                ->Attribute(AZ::ScriptCanvasAttributes::VariableCreationForbidden, AZ::AttributeIsValid::IfPresent)
                ->Method("GetEntityId", &ExecutionState::GetEntityId)
                ->Method("GetScriptCanvasId", &ExecutionState::GetScriptCanvasId)
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
