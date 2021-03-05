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

#pragma once

#include "EMotionFXConfig.h"
#include "AnimGraphObject.h"


namespace EMotionFX
{
    // forward declarations
    class AnimGraphInstance;

    /**
    * AnimGraphTriggerAction is an action that got triggered on specific moment.
    * It can be added to a condition or on a state.
    * This is the base class. If you want to implement a trigger action for a specific purpose, you should
    * write a derived class and implement specific behaviors.
    */
    class EMFX_API AnimGraphTriggerAction
        : public AnimGraphObject
    {
    public:
        AZ_RTTI(AnimGraphTriggerAction, "{D5AE7EBB-7A22-4AF2-93B3-4A7832A2BF50}", AnimGraphObject)
        AZ_CLASS_ALLOCATOR_DECL

        enum EMode : AZ::u8
        {
            MODE_TRIGGERONENTER     = 0,
            MODE_TRIGGERONEXIT      = 1
        };

        AnimGraphTriggerAction();
        virtual ~AnimGraphTriggerAction();

        bool InitAfterLoading(AnimGraph* animGraph) override;

        virtual void TriggerAction(AnimGraphInstance* animGraphInstance) const = 0;
        virtual void Reset(AnimGraphInstance* animGraphInstance)       { MCORE_UNUSED(animGraphInstance); }

        ECategory GetPaletteCategory() const override;

        static void Reflect(AZ::ReflectContext* context);

        EMode GetTriggerMode() const { return m_triggerMode; }

    private:

        static const char* s_modeTriggerOnStart;
        static const char* s_modeTriggerOnExit;
        static const char* s_modeTriggerAtTime;

        EMode              m_triggerMode;
    };
} // namespace EMotionFX

namespace AZ
{
    AZ_TYPE_INFO_SPECIALIZE(EMotionFX::AnimGraphTriggerAction::EMode, "{C3688688-C4BD-482F-A269-FB60AA5E6BEE}");
}