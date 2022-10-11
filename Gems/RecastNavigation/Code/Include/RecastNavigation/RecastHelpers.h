/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Aabb.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/function/function_template.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>

namespace RecastNavigation
{
    //! A helper class to manage different coordinate systems between O3DE and Recast Navigation library.
    //! In O3DE, +Z is up. In Recast library, +Y is up.
    //! The data in this class is kept always in Recast format with +Y. Use @AsVector3WithZup to get a vector in O3DE format.
    class RecastVector3
    {
    public:
        RecastVector3() = default;

        //! A constructor from O3DE coordinate values.
        static RecastVector3 CreateFromVector3SwapYZ(const AZ::Vector3& in)
        {
            RecastVector3 out;
            out.m_xyz[0] = in.GetX(),
            out.m_xyz[1] = in.GetZ(),
            out.m_xyz[2] = in.GetY();
            return out;
        }

        //! A constructor from Recast coordinate values.
        static RecastVector3 CreateFromFloatValuesWithoutAxisSwapping(const float* data)
        {
            RecastVector3 out;
            out.m_xyz[0] = data[0];
            out.m_xyz[1] = data[1];
            out.m_xyz[2] = data[2];
            return out;
        }

        //! @returns raw data without any conversion between coordinate systems. Useful when working with Recast library API.
        float* GetData() { return &m_xyz[0]; }

        //! @returns vector in O3DE coordinate space, with +Z being up. Useful when passing data from Recast to O3DE.
        [[nodiscard]] AZ::Vector3 AsVector3WithZup() const
        {
            return { m_xyz[0], m_xyz[2] , m_xyz[1] };
        }

        float m_xyz[3] = { 0.f, 0.f, 0.f };
    };

    //! A collection of triangle data within a volume defined by an axis aligned bounding box.
    class TileGeometry
    {
    public:
        AZ::Aabb m_worldBounds = AZ::Aabb::CreateNull();
        AZ::Aabb m_scanBounds = AZ::Aabb::CreateNull(); // includes @m_worldBounds and additional border extents

        int m_tileX = 0; // tile coordinate within the navigation grid along X-axis
        int m_tileY = 0; // tile coordinate within the navigation grid along Y-axis

        //! A callback to the async object that requested tile geometry. Useful to return the tile data from a task back to the original caller.
        AZStd::function<void(AZStd::shared_ptr<TileGeometry>)> m_tileCallback;

        //! Indexed vertices.
        AZStd::vector<RecastVector3> m_vertices;
        AZStd::vector<AZ::s32> m_indices;

        //! @returns true if there are no vertices in this tile.
        bool IsEmpty() const
        {
            return m_vertices.empty();
        }
    };

    //! Navigation data in binary Recast form.
    struct NavigationTileData
    {
        //! @returns true if the Recast data is not empty.
        bool IsValid() const
        {
            return m_size > 0 && m_data != nullptr;
        }

        unsigned char* m_data = nullptr;
        int m_size = 0;
    };
} // namespace RecastNavigation
