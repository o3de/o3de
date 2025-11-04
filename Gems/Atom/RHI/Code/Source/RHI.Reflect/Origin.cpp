/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>

#include <Atom/RHI.Reflect/Origin.h>

namespace AZ::RHI
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
