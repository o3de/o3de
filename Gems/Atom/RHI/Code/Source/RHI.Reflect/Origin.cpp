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

#include <AzCore/Serialization/SerializeContext.h>

#include <Atom/RHI.Reflect/Origin.h>

namespace AZ
{
    namespace RHI
    {
        Origin::Origin(uint32_t left, uint32_t top, uint32_t front)
            : m_left{left}
            , m_top{top}
            , m_front{front}
        {}

        void Origin::Reflect(AZ::ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<Origin>()
                    ->Version(0)
                    ->Field("Left", &Origin::m_left)
                    ->Field("Top", &Origin::m_top)
                    ->Field("Front", &Origin::m_front)
                    ;
            }
        }

        bool Origin::operator == (const Origin& rhs) const
        {
            return m_left == rhs.m_left && m_top == rhs.m_top && m_front == rhs.m_front;
        }

        bool Origin::operator != (const Origin& rhs) const
        {
            return m_left != rhs.m_left || m_top != rhs.m_top || m_front != rhs.m_front;
        }
    }
}
