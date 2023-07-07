/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/Base.h>

namespace AZ
{
    class ReflectContext;
}

namespace AZ::RHI
{
    struct Viewport
    {
        AZ_TYPE_INFO(Viewport, "{69160593-B7C3-4E94-A397-CC0A34567698}");
        static void Reflect(AZ::ReflectContext* context);

        Viewport() = default;
        Viewport(
            float minX,
            float maxX,
            float minY,
            float maxY,
            float minZ = 0.0f,
            float maxZ = 1.0f);

        Viewport GetScaled(
            float normalizedMinX,
            float normalizedMaxX,
            float normalizedMinY,
            float normalizedMaxY,
            float normalizedMinZ = 0.0f,
            float normalizedMaxZ = 1.0f) const;

        static Viewport CreateNull();

        bool IsNull() const;

        float m_minX = 0.0f;
        float m_maxX = 0.0f;
        float m_minY = 0.0f;
        float m_maxY = 0.0f;
        float m_minZ = 0.0f;
        float m_maxZ = 1.0f;

        float GetWidth() const;
        float GetHeight() const;
        float GetDepth() const;
    };
}

inline float AZ::RHI::Viewport::GetWidth() const
{
    return m_maxX - m_minX;
}

inline float AZ::RHI::Viewport::GetHeight() const
{
    return m_maxY - m_minY;
}

inline float AZ::RHI::Viewport::GetDepth() const
{
    return m_maxZ - m_minZ;
}
