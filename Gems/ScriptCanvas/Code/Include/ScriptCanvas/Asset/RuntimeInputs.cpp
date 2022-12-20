/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ScriptCanvas/Asset/RuntimeInputs.h>
#include <ScriptCanvas/Core/Nodeable.h>
#include <ScriptCanvas/Grammar/Primitives.h>

namespace ScriptCanvas
{
    enum RuntimeInputsVersion
    {
        RemoveGraphType = 0,
        AddedStaticVariables,
        SupportMemberVariableInputs,
        ExecutionStateSelectionIncludesOnGraphStart,
        Last,
        DoNotVersionRuntimeAssetsBumpTheBuilderVersionInstead
    };

    RuntimeInputs::RuntimeInputs(RuntimeInputs&& rhs)
    {
        *this = AZStd::move(rhs);
    }

    void RuntimeInputs::CopyFrom(const Grammar::ParsedRuntimeInputs& rhs)
    {
        m_nodeables = rhs.m_nodeables;
        m_variables = rhs.m_variables;
        m_entityIds = rhs.m_entityIds;
        m_staticVariables = rhs.m_staticVariables;
    }

    size_t RuntimeInputs::GetConstructorParameterCount() const
    {
        return m_nodeables.size() + m_variables.size() + m_entityIds.size();
    }

    RuntimeInputs& RuntimeInputs::operator=(RuntimeInputs&& rhs)
    {
        if (this != &rhs)
        {
            m_executionSelection = AZStd::move(rhs.m_executionSelection);
            m_nodeables = AZStd::move(rhs.m_nodeables);
            m_variables = AZStd::move(rhs.m_variables);
            m_entityIds = AZStd::move(rhs.m_entityIds);
            m_staticVariables = AZStd::move(rhs.m_staticVariables);
        }

        return *this;
    }

    void RuntimeInputs::Reflect(AZ::ReflectContext* reflectContext)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext))
        {
            serializeContext->Class<RuntimeInputs>()
                ->Version(RuntimeInputsVersion::DoNotVersionRuntimeAssetsBumpTheBuilderVersionInstead)
                ->Field("executionSelection", &RuntimeInputs::m_executionSelection)
                ->Field("nodeables", &RuntimeInputs::m_nodeables)
                ->Field("variables", &RuntimeInputs::m_variables)
                ->Field("entityIds", &RuntimeInputs::m_entityIds)
                ->Field("staticVariables", &RuntimeInputs::m_staticVariables);
        }
    }
} // namespace ScriptCanvas
