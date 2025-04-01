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
    /**
     * The EventDataFloatArray is a type of event data that holds an array of float. The float array is not fixed sized, so it's easy
     * to add and remove elements that make this event type fit for variaty needs.
     * The event data is serialized to motion event as a string, with floats data structed like "n0,n1,n2...n" with comma as a spliter.
     */
    class EMFX_API EventDataFloatArray
        : public EventData
    {
    public:
        AZ_RTTI(EventDataFloatArray, "{8CB47D5E-4C19-42C5-A9E1-FA47DF45163D}", EventData);
        AZ_CLASS_ALLOCATOR_DECL

        EventDataFloatArray() = default;
        ~EventDataFloatArray() override = default;

        static void Reflect(AZ::ReflectContext* context);

        AZ_INLINE const AZStd::string& GetSubject() const { return m_subject;}
        AZ_INLINE float GetElement(size_t index) const;
        AZStd::string DataToString() const;

        bool Equal(const EventData& rhs, bool ignoreEmptyFields = false) const override;

    private:
        AZStd::string m_subject;                    // This can be used as the name of the event.
        AZStd::vector<float> m_floats;              // The actual data of floats.
    };
} // namespace EMotionFX
