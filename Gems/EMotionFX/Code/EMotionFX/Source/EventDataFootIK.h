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

#include <EMotionFX/Source/EventData.h>


namespace AZ
{
    class ReflectContext;
}

namespace EMotionFX
{
    class EMFX_API EventDataFootIK
        : public EventData
    {
    public:
        AZ_RTTI(EventDataFootIK, "{2BF8BB82-F7B1-4833-BB1D-A2537D759E48}", EventData);
        AZ_CLASS_ALLOCATOR_DECL

        enum class Foot : uint32
        {
            Left    = 0,
            Right   = 1,
            Both    = 2
        };

        EventDataFootIK() = default;
        ~EventDataFootIK() override = default;

        static void Reflect(AZ::ReflectContext* context);

        AZ_INLINE Foot GetFoot() const          { return m_foot; }
        AZ_INLINE bool GetIKEnabled() const     { return m_ikEnabled; }
        AZ_INLINE bool GetLocked() const        { return m_locked; }

        bool Equal(const EventData& rhs, bool ignoreEmptyFields = false) const override;

    private:
        Foot    m_foot = Foot::Both;
        bool    m_ikEnabled = true;
        bool    m_locked = false;
    };
}
