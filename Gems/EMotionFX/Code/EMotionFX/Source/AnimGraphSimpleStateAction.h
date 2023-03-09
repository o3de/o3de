/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <EMotionFX/Source/AnimGraphTriggerAction.h>

namespace EMotionFX
{
    // forward declarations
    class AnimGraphInstance;

    /**
     * AnimGraphSimpleStateAction is a specific type of trigger action that modifies the parameter.
     */
    class EMFX_API AnimGraphSimpleStateAction
        : public AnimGraphTriggerAction
    {
    public:
        AZ_RTTI(AnimGraphSimpleStateAction, "{CF719974-AF5D-418C-8F1F-672AB862AFAD}", AnimGraphTriggerAction)
        AZ_CLASS_ALLOCATOR_DECL

        AnimGraphSimpleStateAction() = default;
        AnimGraphSimpleStateAction(AnimGraph* animGraph);
        ~AnimGraphSimpleStateAction() = default;

        bool InitAfterLoading(AnimGraph* animGraph) override;

        void GetSummary(AZStd::string* outResult) const override;
        void GetTooltip(AZStd::string* outResult) const override;
        const char* GetPaletteName() const override;

        void TriggerAction(AnimGraphInstance* animGraphInstance) const override;

        void SetSimpleStateName(const AZStd::string& simpleStateName);
        const AZStd::string& GetSimpleStateName() const { return m_simpleStateName; }

        static void Reflect(AZ::ReflectContext* context);

    private:
        AZStd::string                       m_simpleStateName;
    };
} // namespace EMotionFX
