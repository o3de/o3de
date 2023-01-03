/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/AnimGraphBus.h>
#include <EMotionFX/Source/AnimGraphInstance.h>
#include <EMotionFX/Source/ActorInstance.h>
#include <EMotionFX/Source/AnimGraphSimpleStateAction.h>
#include <EMotionFX/Source/EMotionFXConfig.h>
#include <LmbrCentral/Scripting/SimpleStateComponentBus.h>

namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(AnimGraphSimpleStateAction, AnimGraphAllocator, 0)

    AnimGraphSimpleStateAction::AnimGraphSimpleStateAction()
        : AnimGraphTriggerAction()
    {
    }


    AnimGraphSimpleStateAction::AnimGraphSimpleStateAction(AnimGraph* animGraph)
        : AnimGraphSimpleStateAction()
    {
        InitAfterLoading(animGraph);
    }


    AnimGraphSimpleStateAction::~AnimGraphSimpleStateAction()
    {

    }


    void AnimGraphSimpleStateAction::Reinit()
    {
        
    }


    bool AnimGraphSimpleStateAction::InitAfterLoading(AnimGraph* animGraph)
    {
        if (!AnimGraphTriggerAction::InitAfterLoading(animGraph))
        {
            return false;
        }

        InitInternalAttributesForAllInstances();

        Reinit();
        return true;
    }


    const char* AnimGraphSimpleStateAction::GetPaletteName() const
    {
        return "Simple State Action";
    }


    void AnimGraphSimpleStateAction::TriggerAction(AnimGraphInstance* animGraphInstance) const
    {
        EMotionFX::ActorInstance* actorInstance = animGraphInstance->GetActorInstance();
        if(AZ::Entity* entity = actorInstance->GetEntity())
        {
            LmbrCentral::SimpleStateComponentRequestBus::Event(
                entity->GetId(), &LmbrCentral::SimpleStateComponentRequests::SetState, m_simpleStateName.c_str());
        }
    }

    // Construct and output the information summary string for this object
    void AnimGraphSimpleStateAction::GetSummary(AZStd::string* outResult) const
    {
        *outResult = AZStd::string::format("%s: Simple State Name='%s", RTTI_GetTypeName(), m_simpleStateName.c_str());
    }


    // Construct and output the tooltip for this object
    void AnimGraphSimpleStateAction::GetTooltip(AZStd::string* outResult) const
    {
        AZStd::string columnName, columnValue;

        // add the action type
        columnName = "Action Type: ";
        columnValue = RTTI_GetTypeName();
        *outResult = AZStd::string::format("<table border=\"0\"><tr><td width=\"120\"><b>%s</b></td><td><nobr>%s</nobr></td>", columnName.c_str(), columnValue.c_str());

        // add the parameter
        columnName = "Parameter Name: ";
        *outResult += AZStd::string::format("</tr><tr><td><b><nobr>%s</nobr></b></td><td><nobr>%s</nobr></td>", columnName.c_str(), m_simpleStateName.c_str());
    }

    void AnimGraphSimpleStateAction::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<AnimGraphSimpleStateAction, AnimGraphTriggerAction>()
            ->Version(1)->Field(
            "parameterName", &AnimGraphSimpleStateAction::m_simpleStateName)
            ;

        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (!editContext)
        {
            return;
        }

        editContext->Class<AnimGraphSimpleStateAction>("Simple State Action", "Parameter action attributes")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::AutoExpand, "")
                ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
            ->DataElement(AZ::Edit::UIHandlers::Default, &AnimGraphSimpleStateAction::m_simpleStateName, "SimpleState", "The simple state to transition to.")
            ;
    }
} // namespace EMotionFX
