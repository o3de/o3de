/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <EMotionFX/Source/EMotionFXConfig.h>
#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/AnimGraphInstance.h>
#include <EMotionFX/Source/AnimGraphManager.h>
#include <EMotionFX/Source/AnimGraphTagCondition.h>
#include <EMotionFX/CommandSystem/Source/AnimGraphConditionCommands.h>
#include <MCore/Source/ReflectionSerializer.h>

namespace EMotionFX
{
    const char* AnimGraphTagCondition::s_functionAllTags = "All tags active";
    const char* AnimGraphTagCondition::s_functionOneOrMoreInactive = "One or more tags inactive";
    const char* AnimGraphTagCondition::s_functionOneOrMoreActive = "One or more tags active";
    const char* AnimGraphTagCondition::s_functionNoTagActive = "No tag active";

    AZ_CLASS_ALLOCATOR_IMPL(AnimGraphTagCondition, AnimGraphAllocator)

    AnimGraphTagCondition::AnimGraphTagCondition()
        : AnimGraphTransitionCondition()
        , m_function(FUNCTION_ALL)
    {
    }


    AnimGraphTagCondition::AnimGraphTagCondition(AnimGraph* animGraph)
        : AnimGraphTagCondition()
    {
        InitAfterLoading(animGraph);
    }


    AnimGraphTagCondition::~AnimGraphTagCondition()
    {
    }


    void AnimGraphTagCondition::Reinit()
    {
        const size_t numTags = m_tags.size();
        m_tagParameterIndices.clear();
        m_tagParameterIndices.reserve(numTags);

        // Iterate through the chosen tags in the condition to cache the parameter indices.
        for (size_t i = 0; i < numTags; ++i)
        {
            // Search for the parameter with the name of the tag and save the index.
            const AZ::Outcome<size_t> parameterIndex = m_animGraph->FindValueParameterIndexByName(m_tags[i]);
            if (parameterIndex.IsSuccess())
            {
                // Cache the parameter index to avoid string lookups at runtime.
                m_tagParameterIndices.emplace_back(parameterIndex.GetValue());
            }
            else
            {
                // The tag hasn't been found, add a placeholder so that the indices are still correct.
                m_tagParameterIndices.emplace_back(MCORE_INVALIDINDEX32);
            }
        }
    }


    bool AnimGraphTagCondition::InitAfterLoading(AnimGraph* animGraph)
    {
        if (!AnimGraphTransitionCondition::InitAfterLoading(animGraph))
        {
            return false;
        }

        InitInternalAttributesForAllInstances();
        
        Reinit();
        return true;
    }


    const char* AnimGraphTagCondition::GetPaletteName() const
    {
        return "Tag Condition";
    }

    
    bool AnimGraphTagCondition::TestCondition(AnimGraphInstance* animGraphInstance) const
    {
        // Iterate over the cached tag parameters.
        for (size_t parameterIndex : m_tagParameterIndices)
        {
            if (parameterIndex == MCORE_INVALIDINDEX32)
            {
                // Skip tags that are not present in the anim graph.
                continue;
            }

            // Is the tag active?
            bool tagActive;
            if (!animGraphInstance->GetParameterValueAsBool(static_cast<uint32>(parameterIndex), &tagActive))
            {
                // The parameter is not compatible. Only bool, int and float parameters are compatible.
                continue;
            }

            switch (m_function)
            {
            case FUNCTION_ALL:
            {
                // All tags have to be active, once we hit an inactive one, return failure.
                if (!tagActive)
                {
                    return false;
                }
                break;
            }

            case FUNCTION_ONEORMORE:
            {
                // Return success with the first active tag.
                if (tagActive)
                {
                    return true;
                }
                break;
            }

            case FUNCTION_NONE:
            {
                // No tags should be active. Return failure once with first active tag.
                if (tagActive)
                {
                    return false;
                }
                break;
            }

            case FUNCTION_NOTALL:
            {
                // At least one tag has to be inactive. Once we hit the first inactive one, return success.
                if (!tagActive)
                {
                    return true;
                }
                break;
            }

            default:
            {
                AZ_Assert(false, "Tag condition test function is undefined.");
                return false;
            }
            }
        }

        if (m_function == FUNCTION_ONEORMORE || m_function == FUNCTION_NOTALL)
        {
            return false;
        }

        return true;
    }


    const char* AnimGraphTagCondition::GetTestFunctionString() const
    {
        switch (m_function)
        {
            case FUNCTION_ALL:
            {
                return s_functionAllTags;
            }
            case FUNCTION_NOTALL:
            {
                return s_functionOneOrMoreInactive;
            }
            case FUNCTION_ONEORMORE:
            {
                return s_functionOneOrMoreActive;
            }
            case FUNCTION_NONE:
            {
                return s_functionNoTagActive;
            }
            default:
            {
                return "Unknown test function";
            }
        }
    }


    void AnimGraphTagCondition::CreateTagString(AZStd::string& outTagString) const
    {
        const size_t numTags = m_tags.size();
        if (numTags == 0)
        {
            outTagString += "[]";
            return;
        }

        outTagString = "[";

        for (size_t i = 0; i < numTags; ++i)
        {
            if (i > 0)
            {
                outTagString += ", ";
            }

            outTagString += m_tags[i];
        }

        outTagString += "]";
    }


    void AnimGraphTagCondition::GetSummary(AZStd::string* outResult) const
    {
        *outResult = AZStd::string::format("%s: Test Function='%s', Tags=", RTTI_GetTypeName(), GetTestFunctionString());

        // Add the tags.
        AZStd::string tagString;
        CreateTagString(tagString);
        *outResult += tagString.c_str();
    }


    void AnimGraphTagCondition::GetTooltip(AZStd::string* outResult) const
    {
        AZStd::string columnName, columnValue;

        // Add the condition type.
        columnName = "Condition Type: ";
        columnValue = RTTI_GetTypeName();
        *outResult = AZStd::string::format("<table border=\"0\"><tr><td width=\"165\"><b>%s</b></td><td>%s</td>", columnName.c_str(), columnValue.c_str());

        // Add the tag test function.
        columnName = "Test Function: ";
        columnValue = GetTestFunctionString();
        *outResult += AZStd::string::format("</tr><tr><td><b>%s</b></td><td>%s</td>", columnName.c_str(), columnValue.c_str());

        // Add the tags.
        columnName = "Tags: ";
        CreateTagString(columnValue);
        *outResult += AZStd::string::format("</tr><tr><td><b>%s</b></td><td>%s</td>", columnName.c_str(), columnValue.c_str());
    }

    void AnimGraphTagCondition::SetTags(const AZStd::vector<AZStd::string>& tags)
    {
        m_tags = tags;
    }

    const AZStd::vector<size_t>& AnimGraphTagCondition::GetTagParameterIndices() const
    {
        return m_tagParameterIndices;
    }

    void AnimGraphTagCondition::SetFunction(EFunction function)
    {
        m_function = function;
    }
    
    AZStd::vector<AZStd::string> AnimGraphTagCondition::GetParameters() const
    {
        return m_tags;
    }

    AnimGraph* AnimGraphTagCondition::GetParameterAnimGraph() const
    {
        return GetAnimGraph();
    }

    void AnimGraphTagCondition::ParameterMaskChanged(const AZStd::vector<AZStd::string>& newParameterMask)
    {
        m_tags = newParameterMask;
        Reinit();
    }

    void AnimGraphTagCondition::AddRequiredParameters(AZStd::vector<AZStd::string>& parameterNames) const
    {
        AZ_UNUSED(parameterNames);
        // The parameters are replaceable
    }

    void AnimGraphTagCondition::ParameterAdded(const AZStd::string& newParameterName)
    {
        AZ_UNUSED(newParameterName);
        // Just recompute the indexes in the case the new parameter was inserted before ours
        Reinit();
    }
    
    void AnimGraphTagCondition::ParameterRenamed(const AZStd::string& oldParameterName, const AZStd::string& newParameterName)
    {
        for (AZStd::string& tag : m_tags)
        {
            if (tag == oldParameterName)
            {
                tag = newParameterName;
                // Index doesnt change
            }
        }
    }

    void AnimGraphTagCondition::ParameterOrderChanged(const ValueParameterVector& beforeChange, const ValueParameterVector& afterChange)
    {
        AZ_UNUSED(beforeChange);
        AZ_UNUSED(afterChange);
        // Just recompute the indexes
        Reinit();
    }

    void AnimGraphTagCondition::ParameterRemoved(const AZStd::string& oldParameterName)
    {
        AZ_UNUSED(oldParameterName);
        // Removing a parameter can also shift indexes, so just recompute them
        Reinit();
    }

    void AnimGraphTagCondition::BuildParameterRemovedCommands(MCore::CommandGroup& commandGroup, const AZStd::string& parameterNameToBeRemoved)
    {
        // Only handle in case the removed parameter is a tag from the condition.
        if (AZStd::find(m_tags.begin(), m_tags.end(), parameterNameToBeRemoved) != m_tags.end())
        {
            AZ::Outcome<size_t> conditionIndex = m_transition->FindConditionIndex(this);
            if (conditionIndex.IsSuccess())
            {
                AZStd::vector<AZStd::string> updatedTags = m_tags;
                updatedTags.erase(AZStd::remove(updatedTags.begin(), updatedTags.end(), parameterNameToBeRemoved), updatedTags.end());
                const AZStd::string attributesString = AZStd::string::format("-tags {%s}",
                    MCore::ReflectionSerializer::Serialize(&updatedTags).GetValue().c_str());

                CommandSystem::CommandAdjustTransitionCondition* command = aznew CommandSystem::CommandAdjustTransitionCondition(
                    m_transition->GetAnimGraph()->GetID(),
                    m_transition->GetId(),
                    conditionIndex.GetValue(),
                    attributesString);
                commandGroup.AddCommand(command);
            }
        }
    }

    void AnimGraphTagCondition::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<AnimGraphTagCondition, AnimGraphTransitionCondition>()
            ->Version(1)
            ->Field("function", &AnimGraphTagCondition::m_function)
            ->Field("tags", &AnimGraphTagCondition::m_tags)
            ;


        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (!editContext)
        {
            return;
        }

        editContext->Class<AnimGraphTagCondition>("Tag Condition", "Tag condition attributes")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::AutoExpand, "")
                ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
            ->DataElement(AZ::Edit::UIHandlers::ComboBox, &AnimGraphTagCondition::m_function, "Test Function", "The type of test function or condition.")
                ->EnumAttribute(FUNCTION_ALL,       s_functionAllTags)
                ->EnumAttribute(FUNCTION_NOTALL,    s_functionOneOrMoreInactive)
                ->EnumAttribute(FUNCTION_ONEORMORE, s_functionOneOrMoreActive)
                ->EnumAttribute(FUNCTION_NONE,      s_functionNoTagActive)
            ->DataElement(AZ_CRC_CE("AnimGraphTags"), &AnimGraphTagCondition::m_tags, "Tags", "The tags to watch.")
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &AnimGraphTagCondition::Reinit)
                ->Attribute(AZ_CRC_CE("AnimGraph"), &AnimGraphTagCondition::GetAnimGraph)
                ->Attribute(AZ::Edit::Attributes::ContainerCanBeModified, false)
                ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::HideChildren)
            ;
    }
} // namespace EMotionFX
