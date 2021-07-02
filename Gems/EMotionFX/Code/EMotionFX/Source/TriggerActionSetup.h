/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/containers/vector.h>
#include <EMotionFX/Source/Allocators.h>
#include "AnimGraphObject.h"

namespace AZ
{
    class ReflectContext;
}

namespace EMotionFX
{
    class AnimGraphInstance;
    class AnimGraphTriggerAction;

    class TriggerActionSetup
    {
    public:
        AZ_RTTI(TriggerActionSetup, "{7B4E270C-2C7F-41C4-BFA5-FE6104B789BF}")
        AZ_CLASS_ALLOCATOR_DECL

        TriggerActionSetup();
        virtual ~TriggerActionSetup();

        size_t GetNumActions() const { return m_actions.size(); }
        AnimGraphTriggerAction* GetAction(size_t index) const { return m_actions[index]; }
        AZ::Outcome<size_t> FindActionIndex(AnimGraphTriggerAction* action) const;
        void AddAction(AnimGraphTriggerAction* action);
        void InsertAction(AnimGraphTriggerAction* action, size_t index);
        void ReserveActions(size_t numActions);
        void RemoveAction(size_t index, bool delFromMem = true);
        void RemoveAllActions(bool delFromMem = true);
        void ResetActions(AnimGraphInstance* animGraphInstance);

        AZStd::vector<AnimGraphTriggerAction*>& GetActions() { return m_actions; }
        const AZStd::vector<AnimGraphTriggerAction*>& GetActions() const { return m_actions; }

        static void Reflect(AZ::ReflectContext* context);

    private:
        AZStd::vector<AnimGraphTriggerAction*>          m_actions;
    };
} // namespace EMotionFX
