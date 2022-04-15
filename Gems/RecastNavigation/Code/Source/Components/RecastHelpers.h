/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once
#include <AzCore/Math/Vector3.h>

namespace RecastNavigation
{
    // +Y is up
    class RecastVector3
    {
    public:
        RecastVector3() = default;

        // O3DE coordinate space
        explicit RecastVector3( const AZ::Vector3& in )
        {
            m_x = in.GetX();
            m_y = in.GetZ();
            m_z = in.GetY();
        }

        // Recast coordinate space
        explicit RecastVector3( const float* data )
        {
            m_x = data[0];
            m_y = data[1];
            m_z = data[2];
        }

        float* data() { return &m_x; }

        [[nodiscard]] AZ::Vector3 AsVector3() const
        {
            return { m_x, m_z, m_y };
        }

        float m_x = 0, m_y = 0, m_z = 0;
    };

}
