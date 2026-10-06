/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Crc.h>
#include <AzCore/std/string/string.h>
#include <EMotionFX/Source/EMotionFXConfig.h>
#include <EMotionFX/Source/AnimGraphTransitionCondition.h>

namespace EMotionFX
{
    class AnimGraphInstance;

    // True after a script fires the named event through AnimGraphComponentRequestBus::FireEvent, until a transition takes it or the hold time runs out.
    class EMFX_API AnimGraphScriptEventCondition
        : public AnimGraphTransitionCondition
    {
    public:
        AZ_RTTI(AnimGraphScriptEventCondition, "{15469074-A51C-4C87-8BA7-685171A28328}", AnimGraphTransitionCondition)
        AZ_CLASS_ALLOCATOR_DECL

        AnimGraphScriptEventCondition();
        AnimGraphScriptEventCondition(AnimGraph* animGraph);
        ~AnimGraphScriptEventCondition() override = default;

        void Reinit() override;
        bool InitAfterLoading(AnimGraph* animGraph) override;

        void GetSummary(AZStd::string* outResult) const override;
        void GetTooltip(AZStd::string* outResult) const override;
        const char* GetPaletteName() const override;

        bool TestCondition(AnimGraphInstance* animGraphInstance) const override;

        void SetEventName(const AZStd::string& eventName);
        const AZStd::string& GetEventName() const;
        AZ::Crc32 GetEventId() const { return m_eventId; }

        static void Reflect(AZ::ReflectContext* context);

    private:
        AZStd::string   m_eventName;
        AZ::Crc32       m_eventId;
    };
} // namespace EMotionFX
