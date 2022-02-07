/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <EMotionFX/Source/EventData.h>

namespace AZ
{
    class ReflectContext;
}

namespace EMotionFX::MotionMatching
{
    class EMFX_API DiscardFrameEventData
        : public EventData
    {
    public:
        AZ_RTTI(DiscardFrameEventData, "{25499823-E611-4958-85B7-476BC1918744}", EventData);
        AZ_CLASS_ALLOCATOR_DECL

        DiscardFrameEventData() = default;
        ~DiscardFrameEventData() override = default;

        static void Reflect(AZ::ReflectContext* context);

        bool Equal(const EventData& rhs, bool ignoreEmptyFields = false) const override;

    private:
        AZStd::string m_tag;
    };

    class EMFX_API TagEventData
        : public EventData
    {
    public:
        AZ_RTTI(TagEventData, "{FEFEA2C7-CD68-43B2-94D6-85559E29EABF}", EventData);
        AZ_CLASS_ALLOCATOR_DECL

        TagEventData() = default;
        ~TagEventData() override = default;

        static void Reflect(AZ::ReflectContext* context);

        bool Equal(const EventData& rhs, bool ignoreEmptyFields = false) const override;

    private:
        AZStd::string m_tag;
    };
} // namespace EMotionFX::MotionMatching
