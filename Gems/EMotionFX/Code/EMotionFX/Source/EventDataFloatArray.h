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

namespace EMotionFX
{
    class EMFX_API EventDataFloatArray
        : public EventData
    {
    public:
        AZ_RTTI(EventDataFloatArray, "{8CB47D5E-4C19-42C5-A9E1-FA47DF45163D}", EventData);
        AZ_CLASS_ALLOCATOR_DECL

        EventDataFloatArray() = default;
        ~EventDataFloatArray() override = default;

        static void Reflect(AZ::ReflectContext* context);

        AZ_INLINE const char* GetSubject() const { return m_subject.c_str();}
        AZ_INLINE float GetElement(size_t index) const;
        AZStd::string DataToString() const;

        bool Equal(const EventData& rhs, bool ignoreEmptyFields = false) const override;

    private:
        AZStd::string m_subject;
        AZStd::vector<float> m_floats;
    };
}
