/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include "EMotionFXConfig.h"
#include "AnimGraphTransitionCondition.h"
#include "EventManager.h"
#include "AnimGraph.h"


namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(AnimGraphTransitionCondition, AnimGraphAllocator)

    AnimGraphTransitionCondition::AnimGraphTransitionCondition()
        : AnimGraphObject()
    {
    }

    AnimGraphTransitionCondition::~AnimGraphTransitionCondition()
    {
        if (m_animGraph)
        {
            m_animGraph->RemoveObject(this);
        }
    }

    bool AnimGraphTransitionCondition::InitAfterLoading(AnimGraph* animGraph)
    {
        SetAnimGraph(animGraph);

        if (animGraph)
        {
            animGraph->AddObject(this);
        }

        return true;
    }

    void AnimGraphTransitionCondition::SetTransition(AnimGraphStateTransition* transition)
    {
        m_transition = transition;
    }

    AnimGraphStateTransition* AnimGraphTransitionCondition::GetTransition() const
    {
        return m_transition;
    }
    
    AnimGraphObject::ECategory AnimGraphTransitionCondition::GetPaletteCategory() const
    {
        return AnimGraphObject::CATEGORY_TRANSITIONCONDITIONS;
    }

    void AnimGraphTransitionCondition::GetAttributeStringForAffectedNodeIds(const AZStd::unordered_map<AZ::u64, AZ::u64>& convertedIds, AZStd::string& attributesString) const
    {
        // Default implementation is that the condition is not affected, therefore we dont do anything
        AZ_UNUSED(convertedIds);
        AZ_UNUSED(attributesString);
    }

    void AnimGraphTransitionCondition::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<AnimGraphTransitionCondition, AnimGraphObject>()
            ->Version(1);
    }
} // namespace EMotionFX
