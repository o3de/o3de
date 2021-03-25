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

#include "Nodeable.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <ScriptCanvas/Core/SubgraphInterface.h>
#include <ScriptCanvas/Grammar/GrammarContext.h>
#include <ScriptCanvas/Grammar/GrammarContextBus.h>

namespace NodeableOutCpp
{
    void NoOp(AZ::BehaviorValueParameter* /*result*/, AZ::BehaviorValueParameter* /*arguments*/, int /*numArguments*/) {}
}

namespace ScriptCanvas
{
    using namespace Execution;

    Nodeable::Nodeable()
        : m_noOpFunctor(&NodeableOutCpp::NoOp)
    {}

    Nodeable::Nodeable(ExecutionStateWeakPtr executionState)
        : m_noOpFunctor(&NodeableOutCpp::NoOp)
        , m_executionState(executionState)
    {}   

#if !defined(RELEASE) 
    void Nodeable::CallOut(const AZ::Crc32 key, AZ::BehaviorValueParameter* resultBVP, AZ::BehaviorValueParameter* argsBVPs, int numArguments) const
    {
        GetExecutionOutChecked(key)(resultBVP, argsBVPs, numArguments);
    }
#else
    void Nodeable::CallOut(const AZ::Crc32 key, AZ::BehaviorValueParameter* resultBVP, AZ::BehaviorValueParameter* argsBVPs, int numArguments) const
    {
        GetExecutionOut(key)(resultBVP, argsBVPs, numArguments);
    }
#endif // !defined(RELEASE) 

    void Nodeable::Deactivate()
    {
        OnDeactivate();
    }

    AZ::Data::AssetId Nodeable::GetAssetId() const
    {
        return m_executionState->GetAssetId();
    }

    AZ::EntityId Nodeable::GetEntityId() const
    {
        return m_executionState->GetEntityId();
    }

    AZ::EntityId Nodeable::GetScriptCanvasId() const
    {
        return m_executionState->GetScriptCanvasId();
    }

    void Nodeable::Reflect(AZ::ReflectContext* reflectContext)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext))
        {
            serializeContext->Class<Nodeable>();

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<Nodeable>("Nodeable", "Nodeable")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::ContainerCanBeModified, false)
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ;
            }
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(reflectContext))
        {
            behaviorContext->Class<Nodeable>()
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::List)
                ->Attribute(AZ::ScriptCanvasAttributes::VariableCreationForbidden, AZ::AttributeIsValid::IfPresent)
                ->Constructor<ExecutionStateWeakPtr>()
                ->Method("Deactivate", &Nodeable::Deactivate)
                ->Method("InitializeExecutionState", &Nodeable::InitializeExecutionState)
                ->Method("IsActive", &Nodeable::IsActive)
                ;
        }
    }

    const FunctorOut& Nodeable::GetExecutionOut(AZ::Crc32 key) const
    {
        auto iter = m_outs.find(key);
        AZ_Assert(iter != m_outs.end(), "no out registered for key: %d", key);
        AZ_Assert(iter->second, "null execution methods are not allowed, key: %d", key);
        return iter->second;
    }

    const FunctorOut& Nodeable::GetExecutionOutChecked(AZ::Crc32 key) const
    {
        auto iter = m_outs.find(key);
        
        if (iter == m_outs.end())
        {
            return m_noOpFunctor;
        }
        else if (!iter->second)
        {
            return m_noOpFunctor;
        }
        
        return iter->second;
    }

    void Nodeable::InitializeExecutionState(ExecutionState* executionState)
    {
        AZ_Assert(executionState != nullptr, "execution state for nodeable must not be nullptr");
        AZ_Assert(m_executionState == nullptr, "execution state already initialized");
        m_executionState = executionState->WeakFromThis();
    }

    void Nodeable::InitializeExecutionOuts(const AZ::Crc32* begin, const AZ::Crc32* end)
    {
        m_outs.reserve(end - begin);

        for (; begin != end; ++begin)
        {
            SetExecutionOut(*begin, AZStd::move(FunctorOut(&NodeableOutCpp::NoOp)));
        }
    }

    void Nodeable::InitializeExecutionOuts(const AZStd::vector<AZ::Crc32>& keys)
    {
        InitializeExecutionOuts(keys.begin(), keys.end());
    }

    void Nodeable::SetExecutionOut(AZ::Crc32 key, FunctorOut&& out)
    {
        AZ_Assert(out, "null executions methods are not allowed, key: %d", key);
        m_outs[key] = AZStd::move(out);
    }

    void Nodeable::SetExecutionOutChecked(AZ::Crc32 key, FunctorOut&& out)
    {
        if (!out)
        {
            AZ_Error("ScriptCanvas", false, "null executions methods are not allowed, key: %d", key);
            return;
        }

        SetExecutionOut(key, AZStd::move(out));
    }
}
