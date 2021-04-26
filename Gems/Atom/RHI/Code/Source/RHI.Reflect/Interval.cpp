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

#include <Atom/RHI.Reflect/Interval.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AZ
{
    namespace RHI
    {
        void Interval::Reflect(AZ::ReflectContext* context)
        {
            if (SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<Interval>()
                    ->Version(1)
                    ->Field("m_min", &Interval::m_min)
                    ->Field("m_max", &Interval::m_max);
            }
        }

        Interval::Interval(uint32_t min, uint32_t max)
            : m_min{min}
            , m_max{max}
        {}

        bool Interval::operator == (const Interval& rhs) const
        {
            return m_min == rhs.m_min && m_max == rhs.m_max;
        }

        bool Interval::operator != (const Interval& rhs) const
        {
            return m_min != rhs.m_min || m_max != rhs.m_max;
        }
    }
}
