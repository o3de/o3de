/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI.Reflect/Viewport.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AZ::RHI
{
    void Viewport::Reflect(AZ::ReflectContext* context)
    {
        if (SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context))
        {
            serializeContext->Class<Viewport>()
                ->Version(1)
                ->Field("m_minX", &Viewport::m_minX)
                ->Field("m_maxX", &Viewport::m_maxX)
                ->Field("m_minY", &Viewport::m_minY)
                ->Field("m_maxY", &Viewport::m_maxY)
                ->Field("m_minZ", &Viewport::m_minZ)
                ->Field("m_maxZ", &Viewport::m_maxZ);
        }
    }

    Viewport::Viewport(
        float minX,
        float maxX,
        float minY,
        float maxY,
        float minZ,
        float maxZ)
        : m_minX(minX)
        , m_maxX(maxX)
        , m_minY(minY)
        , m_maxY(maxY)
        , m_minZ(minZ)
        , m_maxZ(maxZ)
    {}

    Viewport Viewport::GetScaled(
        float normalizedMinX,
        float normalizedMaxX,
        float normalizedMinY,
        float normalizedMaxY,
        float normalizedMinZ,
        float normalizedMaxZ) const
    {
        Viewport viewport;
        viewport.m_minX = Lerp(m_minX, m_maxX, normalizedMinX);
        viewport.m_maxX = Lerp(m_minX, m_maxX, normalizedMaxX);
        viewport.m_minY = Lerp(m_minY, m_maxY, normalizedMinY);
        viewport.m_maxY = Lerp(m_minY, m_maxY, normalizedMaxY);
        viewport.m_minZ = Lerp(m_minZ, m_maxZ, normalizedMinZ);
        viewport.m_maxZ = Lerp(m_minZ, m_maxZ, normalizedMaxZ);
        return viewport;
    }

    Viewport Viewport::CreateNull()
    {
        return Viewport{0.f, 0.f, 0.f, 0.f, 0.f, 0.f};
    }

    bool Viewport::IsNull() const
    {
        return ((m_minX >= m_maxX) ||
            (m_minY >= m_maxY) ||
            (m_minZ >= m_maxZ));
    }
}
