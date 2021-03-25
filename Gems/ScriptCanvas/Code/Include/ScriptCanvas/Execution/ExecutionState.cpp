/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
    ExecutionStateConfig::ExecutionStateConfig(AZ::Data::Asset<RuntimeAsset> runtimeAsset, RuntimeComponent& component)
        : asset(runtimeAsset)
        , component(component)
        , runtimeData(runtimeAsset.Get()->GetData())
    {}

    ExecutionState::ExecutionState(const ExecutionStateConfig& config)
        : m_component(&config.component)
    {}
    
    ExecutionStatePtr ExecutionState::Create(const ExecutionStateConfig& config)
    {
       switch (config.runtimeData.m_input.m_execution)
       {
       case ExecutionMode::Interpreted:
       {
           if (config.runtimeData.m_script.GetId().IsValid())
           {
               switch (config.runtimeData.m_input.m_executionCharacteristics)
               {
               case Grammar::ExecutionCharacteristics::PerEntity:
                   return AZStd::make_shared<ExecutionStateInterpretedPerActivation>(config);

               case Grammar::ExecutionCharacteristics::Pure:
                   return AZStd::make_shared<ExecutionStateInterpretedPure>(config);

               default:
                   break;
               }
           }
       }
       break;

       case ExecutionMode::Native:
       default:
           AZ_Assert(false, "not done yet!");
           return nullptr;
           ;
       } // switch (config.executionMode)

       return nullptr;
    }
    
    AZ::Data::AssetId ExecutionState::GetAssetId() const
    {
        return m_component->GetAsset().GetId();
    }

    AZ::EntityId ExecutionState::GetEntityId() const
    {
        return m_component->GetEntityId();
    }

    AZ::ComponentId ExecutionState::GetComponentId() const
    {
        return m_component->GetId();
    }

    GraphIdentifier ExecutionState::GetGraphIdentifier() const
    {
        return  m_component->GetGraphIdentifier();
    }

    GraphIdentifier ExecutionState::GetGraphIdentifier(const AZ::Data::AssetId& id) const
    {
        return GraphIdentifier(AZ::Data::AssetId(id.m_guid, 0), GetComponentId());
    }

    AZ::EntityId ExecutionState::GetScriptCanvasId() const
    {
        return m_component->GetScriptCanvasId();
    }

    void ExecutionState::Reflect(AZ::ReflectContext* reflectContext)
    {
        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(reflectContext))
        {
            behaviorContext->Class<ExecutionState>()
                ->Method("GetEntityId", &ExecutionState::GetEntityId)
                ->Method("GetScriptCanvasId", &ExecutionState::GetScriptCanvasId)
                ->Method("ToString", &ExecutionState::ToString)
                    ->Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::ToString)
                ;
        }

        ExecutionStateInterpreted::Reflect(reflectContext);
        ExecutionStateInterpretedPerActivation::Reflect(reflectContext);
        ExecutionStateInterpretedPure::Reflect(reflectContext);
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
