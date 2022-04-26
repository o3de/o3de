/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once
#include <DetourAlloc.h>
#include <Recast.h>
#include <AzCore/Math/Aabb.h>
#include <AzCore/Math/Vector3.h>

namespace RecastNavigation
{
    //! A helper class to manage different coordinate systems between O3DE and Recast Navigation library.
    //! In O3DE, +Z is up. In Recast library, +Y is up.
    class RecastVector3
    {
    public:
        RecastVector3() = default;

        //! A constructor from O3DE coordinate values.
        explicit RecastVector3(const AZ::Vector3& in)
        {
            m_x = in.GetX();
            m_y = in.GetZ(); // swapping y and z
            m_z = in.GetY();
        }

        //! A constructor from Recast coordinate values.
        explicit RecastVector3(const float* data)
        {
            m_x = data[0];
            m_y = data[1];
            m_z = data[2];
        }
        
        //! @return raw data without any conversion between coordinate systems.
        float* data() { return &m_x; }
        
        //! @return vector in O3DE coordinate space, with +Z being up
        [[nodiscard]] AZ::Vector3 AsVector3() const
        {
            return { m_x, m_z, m_y };
        }

        float m_x = 0, m_y = 0, m_z = 0;
    };

    class RecastCustomContext final : public rcContext
    {
    public:
        void doLog(const rcLogCategory, const char* message, [[maybe_unused]] const int messageLength) override
        {
            AZ_Printf("Recast", "%s", message);
        }
    };
    
    //! A collection of triangle data within a volume defined by an axis aligned bounding box.
    class BoundedGeometry
    {
    public:
        AZ::Aabb m_worldBounds = AZ::Aabb::CreateNull();
        AZStd::vector<RecastVector3> m_verts;
        AZStd::vector<AZ::s32> m_indices;

        bool IsEmpty() const
        {
            return m_verts.empty();
        }

        //! Reset and clear geometry and the volume.
        void clear()
        {
            m_worldBounds.SetNull();
            m_verts.clear();
            m_indices.clear();
        }
    };
    
    //! Navigation data in binary Recast form
    class NavigationTileData
    {
    public:
        void Free()
        {
            dtFree(m_data);
        }

        bool IsValid() const
        {
            return m_size > 0 && m_data != nullptr;
        }

        unsigned char* m_data = nullptr;
        int m_size = 0;
    };
}
