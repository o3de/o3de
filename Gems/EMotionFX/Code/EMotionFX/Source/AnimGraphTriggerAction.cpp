/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include "EMotionFXConfig.h"
#include "AnimGraphTriggerAction.h"
#include "EventManager.h"
#include "AnimGraph.h"


namespace EMotionFX
{
    const char* AnimGraphTriggerAction::s_modeTriggerOnStart = "On Enter";
    const char* AnimGraphTriggerAction::s_modeTriggerOnExit = "On Exit";

    AZ_CLASS_ALLOCATOR_IMPL(AnimGraphTriggerAction, AnimGraphAllocator)

    AnimGraphTriggerAction::AnimGraphTriggerAction()
        : AnimGraphObject()
        , m_triggerMode(MODE_TRIGGERONENTER)
    {
    }


    AnimGraphTriggerAction::~AnimGraphTriggerAction()
    {
        if (m_animGraph)
        {
            m_animGraph->RemoveObject(this);
        }
    }


    bool AnimGraphTriggerAction::InitAfterLoading(AnimGraph* animGraph)
    {
        SetAnimGraph(animGraph);

        if (animGraph)
        {
            animGraph->AddObject(this);
        }

        return true;
    }
    

    AnimGraphObject::ECategory AnimGraphTriggerAction::GetPaletteCategory() const
    {
        return AnimGraphObject::CATEGORY_TRIGGERACTIONS;
    }


    void AnimGraphTriggerAction::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<AnimGraphTriggerAction, AnimGraphObject>()
            ->Version(1)
            ->Field("triggerMode", &AnimGraphTriggerAction::m_triggerMode)
            ;

        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (!editContext)
        {
            return;
        }

        editContext->Enum<EMode>("Trigger Mode", "The timing when the action will be triggered.")
            ->Value(s_modeTriggerOnStart, MODE_TRIGGERONENTER)
            ->Value(s_modeTriggerOnExit, MODE_TRIGGERONEXIT)
            ;

        editContext->Class<AnimGraphTriggerAction>("Parameter Action", "Parameter action attributes")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
            ->Attribute(AZ::Edit::Attributes::AutoExpand, "")
            ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
            ->DataElement(AZ::Edit::UIHandlers::ComboBox, &AnimGraphTriggerAction::m_triggerMode, "Trigger Mode", "The timing when the action will be triggered.")
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &AnimGraphTriggerAction::Reinit)
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
            ;
    }
} // namespace EMotionFX
