/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <EMotionFX/Source/AnimGraphInstance.h>
#include <EMotionFX/Source/AnimGraphScriptEventCondition.h>
#include <EMotionFX/Source/EMotionFXConfig.h>

namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(AnimGraphScriptEventCondition, AnimGraphConditionAllocator)

    AnimGraphScriptEventCondition::AnimGraphScriptEventCondition()
        : AnimGraphTransitionCondition()
    {
    }

    AnimGraphScriptEventCondition::AnimGraphScriptEventCondition(AnimGraph* animGraph)
        : AnimGraphScriptEventCondition()
    {
        InitAfterLoading(animGraph);
    }

    void AnimGraphScriptEventCondition::Reinit()
    {
        m_eventId = AZ::Crc32(m_eventName.c_str());
    }

    bool AnimGraphScriptEventCondition::InitAfterLoading(AnimGraph* animGraph)
    {
        if (!AnimGraphTransitionCondition::InitAfterLoading(animGraph))
        {
            return false;
        }

        InitInternalAttributesForAllInstances();

        Reinit();
        return true;
    }

    const char* AnimGraphScriptEventCondition::GetPaletteName() const
    {
        return "Script Event Condition";
    }

    bool AnimGraphScriptEventCondition::TestCondition(AnimGraphInstance* animGraphInstance) const
    {
        return !m_eventName.empty() && animGraphInstance->IsScriptEventActive(m_eventId);
    }

    void AnimGraphScriptEventCondition::GetSummary(AZStd::string* outResult) const
    {
        *outResult = AZStd::string::format("%s: Event='%s'", RTTI_GetTypeName(), m_eventName.c_str());
    }

    void AnimGraphScriptEventCondition::GetTooltip(AZStd::string* outResult) const
    {
        *outResult = AZStd::string::format("<table border=\"0\"><tr><td width=\"165\"><b>Condition Type: </b></td><td>%s</td></tr><tr><td><b>Event: </b></td><td>%s</td></tr></table>",
            RTTI_GetTypeName(), m_eventName.c_str());
    }

    void AnimGraphScriptEventCondition::SetEventName(const AZStd::string& eventName)
    {
        m_eventName = eventName;
        Reinit();
    }

    const AZStd::string& AnimGraphScriptEventCondition::GetEventName() const
    {
        return m_eventName;
    }

    void AnimGraphScriptEventCondition::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<AnimGraphScriptEventCondition, AnimGraphTransitionCondition>()
            ->Version(1)
            ->Field("eventName", &AnimGraphScriptEventCondition::m_eventName)
            ;

        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (!editContext)
        {
            return;
        }

        editContext->Class<AnimGraphScriptEventCondition>("Script Event Condition", "Script event condition attributes")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::AutoExpand, "")
                ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
            ->DataElement(AZ::Edit::UIHandlers::Default, &AnimGraphScriptEventCondition::m_eventName, "Event Name", "The event name a script fires with FireEvent. The condition stays true for a short hold time after it was fired, until a transition takes it.")
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &AnimGraphScriptEventCondition::Reinit)
            ;
    }
} // namespace EMotionFX
