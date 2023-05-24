/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "GroupParameter.h"

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <EMotionFX/Source/Allocators.h>


namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(GroupParameter, AnimGraphAllocator)


    GroupParameter::~GroupParameter()
    {
        for (Parameter* param : m_childParameters)
        {
            delete param;
        }
    }

    void GroupParameter::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<GroupParameter, Parameter>()
            ->Version(1)
            ->Field("childParameters", &GroupParameter::m_childParameters)
            ;

        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (!editContext)
        {
            return;
        }

        editContext->Class<GroupParameter>("Group parameter", "")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
            ;
    }

    const char* GroupParameter::GetTypeDisplayName() const
    {
        return "Group";
    }

    size_t GroupParameter::GetNumValueParameters() const
    {
        size_t numValueParameters = 0;
        for (const Parameter* parameter : m_childParameters)
        {
            if (azrtti_typeid(parameter) != azrtti_typeid<GroupParameter>())
            {
                ++numValueParameters;
            }
            else
            {
                const GroupParameter* childGroup = static_cast<const GroupParameter*>(parameter);
                numValueParameters += childGroup->GetNumValueParameters();
            }
        }
        return numValueParameters;
    }

    size_t GroupParameter::GetNumParameters() const
    {
        size_t numValueParameters = 0;
        for (const Parameter* parameter : m_childParameters)
        {
            ++numValueParameters;
            if (azrtti_typeid(parameter) == azrtti_typeid<GroupParameter>())
            {
                const GroupParameter* childGroup = static_cast<const GroupParameter*>(parameter);
                numValueParameters += childGroup->GetNumParameters();
            }
        }
        return numValueParameters;
    }

    const Parameter* GroupParameter::FindParameter(size_t index) const
    {
        for (const Parameter* childParam : m_childParameters)
        {
            if (index != 0)
            {
                --index;
                if (azrtti_typeid(childParam) == azrtti_typeid<GroupParameter>())
                {
                    const GroupParameter* childGroupParameter = static_cast<const GroupParameter*>(childParam);
                    const Parameter* foundParam = childGroupParameter->FindParameter(index);
                    if (foundParam)
                    {
                        return foundParam;
                    }
                    else
                    {
                        index -= childGroupParameter->GetNumParameters();
                    }
                }
            }
            else
            {
                return childParam;
            }
        }
        return nullptr;
    }

    ParameterVector GroupParameter::RecursivelyGetChildParameters() const
    {
        ParameterVector childParameters;
        for (const Parameter* parameter : m_childParameters)
        {
            childParameters.push_back(const_cast<Parameter*>(parameter));
            if (azrtti_typeid(parameter) == azrtti_typeid<GroupParameter>())
            {
                const GroupParameter* childGroup = static_cast<const GroupParameter*>(parameter);
                const ParameterVector recursiveChildParameters = childGroup->RecursivelyGetChildParameters();
                AZStd::copy(recursiveChildParameters.begin(), recursiveChildParameters.end(), AZStd::back_inserter(childParameters));
            }
        }
        
        return childParameters;
    }

    GroupParameterVector GroupParameter::RecursivelyGetChildGroupParameters() const
    {
        GroupParameterVector childParameters;
        for (const Parameter* parameter : m_childParameters)
        {
            if (azrtti_typeid(parameter) == azrtti_typeid<GroupParameter>())
            {
                childParameters.push_back(static_cast<GroupParameter*>(const_cast<Parameter*>(parameter)));
                const GroupParameter* childGroup = static_cast<const GroupParameter*>(parameter);
                const GroupParameterVector recursiveChildParameters = childGroup->RecursivelyGetChildGroupParameters();
                AZStd::copy(recursiveChildParameters.begin(), recursiveChildParameters.end(), AZStd::back_inserter(childParameters));
            }
        }
        return childParameters;
    }

    ValueParameterVector GroupParameter::RecursivelyGetChildValueParameters() const
    {
        ValueParameterVector childParameters;
        for (const Parameter* parameter : m_childParameters)
        {
            if (azrtti_typeid(parameter) != azrtti_typeid<GroupParameter>())
            {
                childParameters.push_back(static_cast<ValueParameter*>(const_cast<Parameter*>(parameter)));
            }
            else
            {
                const GroupParameter* childGroup = static_cast<const GroupParameter*>(parameter);
                const ValueParameterVector recursiveChildParameters = childGroup->RecursivelyGetChildValueParameters();
                AZStd::copy(recursiveChildParameters.begin(), recursiveChildParameters.end(), AZStd::back_inserter(childParameters));
            }
        }
        return childParameters;
    }

    ValueParameterVector GroupParameter::GetChildValueParameters() const
    {
        ValueParameterVector childParameters;
        for (const Parameter* parameter : m_childParameters)
        {
            if (azrtti_typeid(parameter) != azrtti_typeid<GroupParameter>())
            {
                childParameters.push_back(static_cast<ValueParameter*>(const_cast<Parameter*>(parameter)));
            }
        }
        return childParameters;
    }

    const ParameterVector& GroupParameter::GetChildParameters() const
    {
        return m_childParameters;

    }

    const Parameter* GroupParameter::FindParameterByName(const AZStd::string& parameterName) const
    {
        if (parameterName == GetName())
        {
            return this;
        }
        else
        {
            for (const Parameter* childParam : m_childParameters)
            {
                if (azrtti_typeid(childParam) == azrtti_typeid<GroupParameter>())
                {
                    const Parameter* childGroupParam = static_cast<const GroupParameter*>(childParam)->FindParameterByName(parameterName);
                    if (childGroupParam)
                    {
                        return childGroupParam;
                    }
                }
                else
                {
                    if (parameterName == childParam->GetName())
                    {
                        return childParam;
                    }
                }
            }
        }
        return nullptr; // not found
    }

    const GroupParameter* GroupParameter::FindGroupParameterByName(const AZStd::string& groupParameterName) const
    {
        if (groupParameterName == GetName())
        {
            return this;
        }
        else
        {
            for (const Parameter* childParam : m_childParameters)
            {
                if (azrtti_typeid(childParam) == azrtti_typeid<GroupParameter>())
                {
                    const GroupParameter* childGroupParam = static_cast<const GroupParameter*>(childParam)->FindGroupParameterByName(groupParameterName);
                    if (childGroupParam)
                    {
                        return childGroupParam;
                    }
                }
            }
        }
        return nullptr; // not found
    }

    AZ::Outcome<size_t> GroupParameter::FindParameterIndexByName(const AZStd::string& parameterName) const
    {
        size_t relativeIndex = 0; // index relative to this group parameter
        for (const Parameter* childParam : m_childParameters)
        {
            if (childParam->GetName() == parameterName)
            {
                return AZ::Success(relativeIndex);
            }
            ++relativeIndex;
            if (azrtti_typeid(childParam) == azrtti_typeid<GroupParameter>())
            {
                const GroupParameter* childGroupParameter = static_cast<const GroupParameter*>(childParam);
                AZ::Outcome<size_t> relativeIndexFromChild = childGroupParameter->FindParameterIndexByName(parameterName);
                if (relativeIndexFromChild.IsSuccess())
                {
                    relativeIndex += relativeIndexFromChild.GetValue();
                    return AZ::Success(relativeIndex);
                }
                else
                {
                    relativeIndex += childGroupParameter->GetNumParameters();
                }
            }
        }
        return AZ::Failure();
    }

    AZ::Outcome<size_t> GroupParameter::FindValueParameterIndexByName(const AZStd::string& parameterName) const
    {
        size_t relativeIndex = 0; // index relative to this group parameter
        for (const Parameter* childParam : m_childParameters)
        {
            if (azrtti_typeid(childParam) == azrtti_typeid<GroupParameter>())
            {
                const GroupParameter* childGroupParameter = static_cast<const GroupParameter*>(childParam);
                AZ::Outcome<size_t> relativeIndexFromChild = childGroupParameter->FindValueParameterIndexByName(parameterName);
                if (relativeIndexFromChild.IsSuccess())
                {
                    relativeIndex += relativeIndexFromChild.GetValue();
                    return AZ::Success(relativeIndex);
                }
                else
                {
                    relativeIndex += childGroupParameter->GetNumValueParameters();
                }
            }
            else if (childParam->GetName() == parameterName)
            {
                return AZ::Success(relativeIndex);
            }
            else
            {
                ++relativeIndex;
            }
        }
        return AZ::Failure();
    }

    AZ::Outcome<size_t> GroupParameter::FindValueParameterIndex(const Parameter* valueParameter) const
    {
        size_t relativeIndex = 0; // index relative to this group parameter
        for (const Parameter* childParam : m_childParameters)
        {
            if (azrtti_typeid(childParam) == azrtti_typeid<GroupParameter>())
            {
                const GroupParameter* childGroupParameter = static_cast<const GroupParameter*>(childParam);
                AZ::Outcome<size_t> relativeIndexFromChild = childGroupParameter->FindValueParameterIndex(valueParameter);
                if (relativeIndexFromChild.IsSuccess())
                {
                    relativeIndex += relativeIndexFromChild.GetValue();
                    return AZ::Success(relativeIndex);
                }
                else
                {
                    relativeIndex += childGroupParameter->GetNumValueParameters();
                }
            }
            else if (childParam == valueParameter)
            {
                return AZ::Success(relativeIndex);
            }
            else
            {
                ++relativeIndex;
            }
        }
        return AZ::Failure();
    }

    AZ::Outcome<size_t> GroupParameter::FindParameterIndex(const Parameter* parameter) const
    {
        size_t relativeIndex = 0; // index relative to this group parameter
        for (const Parameter* childParam : m_childParameters)
        {
            if (childParam == parameter)
            {
                return AZ::Success(relativeIndex);
            }
            else if (azrtti_typeid(childParam) == azrtti_typeid<GroupParameter>())
            {
                const GroupParameter* childGroupParameter = static_cast<const GroupParameter*>(childParam);
                AZ::Outcome<size_t> relativeIndexFromChild = childGroupParameter->FindParameterIndex(parameter);
                if (relativeIndexFromChild.IsSuccess())
                {
                    // Add 1 for the group parameter itself.
                    relativeIndex += relativeIndexFromChild.GetValue() + 1;
                    return AZ::Success(relativeIndex);
                }
                else
                {
                    relativeIndex += childGroupParameter->GetNumParameters();
                }
            }
            ++relativeIndex;
        }
        return AZ::Failure();
    }

    AZ::Outcome<size_t> GroupParameter::FindRelativeParameterIndex(const Parameter* parameter) const
    {
        const size_t parameterCount = m_childParameters.size();
        for (size_t i = 0; i < parameterCount; ++i)
        {
            if (m_childParameters[i] == parameter)
            {
                return AZ::Success(i);
            }
        }
        return AZ::Failure();
    }

    const GroupParameter* GroupParameter::FindParentGroupParameter(const Parameter* parameter) const
    {
        for (const Parameter* childParam : m_childParameters)
        {
            if (childParam == parameter)
            {
                return this;
            }
            else if (azrtti_typeid(childParam) == azrtti_typeid<GroupParameter>())
            {
                const GroupParameter* childGroupParameter = static_cast<const GroupParameter*>(childParam);
                const GroupParameter* groupParameter = childGroupParameter->FindParentGroupParameter(parameter);
                if (groupParameter)
                {
                    return groupParameter;
                }
            }
        }
        return nullptr;
    }

    bool GroupParameter::AddParameter(Parameter* parameter, const GroupParameter* parent)
    {
        if (parent == this)
        {
            m_childParameters.emplace_back(parameter);
            return true;
        }
        else
        {
            for (Parameter* childParam : m_childParameters)
            {
                if (azrtti_typeid(childParam) == azrtti_typeid<GroupParameter>())
                {
                    GroupParameter* groupParameter = static_cast<GroupParameter*>(childParam);
                    if (groupParameter->AddParameter(parameter, parent))
                    {
                        return true;
                    }
                }
            }
        }
        return false;
    }

    bool GroupParameter::InsertParameter(size_t index, Parameter* parameter, const GroupParameter* parent)
    {
        if (parent == this)
        {
            // insert it relative to this parent
            if (index <= m_childParameters.size())
            {
                m_childParameters.insert(m_childParameters.begin() + index, parameter);
                return true;
            }
            return false;
        }
        else
        {
            // Keep searching for the right parent
            for (Parameter* childParam : m_childParameters)
            {
                if (azrtti_typeid(childParam) == azrtti_typeid<GroupParameter>())
                {
                    GroupParameter* groupParameter = static_cast<GroupParameter*>(childParam);
                    if (groupParameter->InsertParameter(index, parameter, parent))
                    {
                        return true;
                    }
                }
            }
        }
        return false;
    }

    bool GroupParameter::RemoveParameter(Parameter* parameter)
    {
        for (ParameterVector::iterator it = m_childParameters.begin(); it != m_childParameters.end(); ++it)
        {
            if (*it != parameter)
            {
                if (azrtti_typeid(*it) == azrtti_typeid<GroupParameter>())
                {
                    GroupParameter* childGroupParameter = static_cast<GroupParameter*>(*it);
                    if (childGroupParameter->RemoveParameter(parameter))
                    {
                        return true;
                    }
                }
            }
            else
            {
                const Parameter* param = *it;
                m_childParameters.erase(it);
                delete param;
                return true;
            }
        }
        return false;
    }

    bool GroupParameter::TakeParameterFromParent(const Parameter* parameter)
    {
        for (ParameterVector::iterator it = m_childParameters.begin(); it != m_childParameters.end(); ++it)
        {
            if (*it != parameter)
            {
                if (azrtti_typeid(*it) == azrtti_typeid<GroupParameter>())
                {
                    GroupParameter* childGroupParameter = static_cast<GroupParameter*>(*it);
                    if (childGroupParameter->TakeParameterFromParent(parameter))
                    {
                        return true;
                    }
                }
            }
            else
            {
                m_childParameters.erase(it);
                return true;
            }
        }
        return false;
    }

}   // namespace EMotionFX
