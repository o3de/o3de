/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <EMotionFX/Source/AnimGraphTriggerAction.h>
#include <EMotionFX/Source/TriggerActionSetup.h>

namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(TriggerActionSetup, EMotionFX::ActorAllocator)

    TriggerActionSetup::TriggerActionSetup()
    {
    }

    TriggerActionSetup::~TriggerActionSetup()
    {
        RemoveAllActions();
    }

    AZ::Outcome<size_t> TriggerActionSetup::FindActionIndex(AnimGraphTriggerAction* action) const
    {
        const auto iterator = AZStd::find(m_actions.begin(), m_actions.end(), action);
        if (iterator == m_actions.end())
        {
            return AZ::Failure();
        }

        return AZ::Outcome<size_t>(iterator - m_actions.begin());
    }

    void TriggerActionSetup::AddAction(AnimGraphTriggerAction* action)
    {
        m_actions.push_back(action);
    }

    void TriggerActionSetup::InsertAction(AnimGraphTriggerAction* action, size_t index)
    {
        m_actions.insert(m_actions.begin() + index, action);
    }

    void TriggerActionSetup::ReserveActions(size_t numAction)
    {
        m_actions.reserve(numAction);
    }

    void TriggerActionSetup::RemoveAction(size_t index, bool delFromMem)
    {
        if (delFromMem)
        {
            delete m_actions[index];
        }

        m_actions.erase(m_actions.begin() + index);
    }

    void TriggerActionSetup::RemoveAllActions(bool delFromMem)
    {
        // delete them all from memory
        if (delFromMem)
        {
            for (AnimGraphTriggerAction* action : m_actions)
            {
                delete action;
            }
        }

        m_actions.clear();
    }

    void TriggerActionSetup::ResetActions(AnimGraphInstance* animGraphInstance)
    {
        for (AnimGraphTriggerAction* action : m_actions)
        {
            action->Reset(animGraphInstance);
        }
    }

    void TriggerActionSetup::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<TriggerActionSetup>()
            ->Version(1)
            ->Field("actions", &TriggerActionSetup::m_actions)
        ;

        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (!editContext)
        {
            return;
        }
    }
} // namespace EMotionFX
