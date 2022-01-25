/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/base.h>
#include <AzCore/Math/Aabb.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/std/containers/vector.h>
#include <TerrainRenderer/Vector2i.h>
#include <TerrainRenderer/Aabb2i.h>

namespace Terrain
{

    struct ClipmapBoundsDescritor
    {
        //! Size of the clipmap in width / height.
        uint32_t m_size = 1024;

        //! Current center location of the clipmap
        AZ::Vector2 m_center = AZ::Vector2::CreateZero();

        //! Unused area around edges of the clipmap. Center point must be moved by
        //! at least this much before an update is triggered. This allows for larger
        //! but less frequent updates.
        uint32_t m_margin = 4;

        //! Scale of the clip map compared to the world. A scale of 0.5 means that
        //! a clipmap of size 1024 would cover 512 meters.
        float m_scale = 1.0f;
    };

    struct ClipmapBoundsRegion
    {
        //! The world bounds of the updated region. Z is ignored.
        AZ::Aabb m_worldAabb;

        //! The clipmaps bounds of the updated region. Will always be between 0 and size.
        //! Min inclusive, max exclusive. i.e., a single pixel would be min [0, 0], max [1, 1].
        Aabb2i m_localAabb;

        bool operator==(const ClipmapBoundsRegion& other) const;
        bool operator!=(const ClipmapBoundsRegion& other) const;
    };

    // This class manages a single clipmap region. This is typically a texture which contains data
    // around a point like the camera. The underlying texture never actually moves, and since it
    // wraps it can be thought of like a repeating grid. This makes looking up data in the clipmap
    // trivial since it's just the world coordinate scaled by some amount, however only data inside
    // the clipmap bounds will actually be valid. This technique also allows for only the edge areas
    // of the clipmap to be updated as the center point moves around the world.
    //
    // The edges of the clipmap region will typically run through the texture, dividing it into 4
    // regions, except in cases where the clipmap region happens to be aligned with the underlying
    // grid. This means whenever some bounding box needs to be updated in the clipmap, it may actually
    // translate to 4 different areas of the underlying texture - one for each quadrant.
    //
    // This class aids in figuring out which areas of a clipmap need to be updated as its center point
    // moves around in the world, and can map a single region that needs to be updated into several
    // separate regions for each quadrant.
    // _____________________________
    // |      |      |      |      |      Clipmap
    // |      |      |      |      |      Texture
    // |______|______|______|______|      ______ 
    // |      |     _|____  |      |     | |    |
    // |      |    | |    | |      |     |_|____|
    // |______|____|_|_*__|_|______|     |_|____|
    // |      |    |_|____| |      |
    // |      |      |      |      |
    // |______|______|______|______|
    // |      |      |      |      |
    // |      |      |      |      |
    // |______|______|______|______|

    class ClipmapBounds
    {
    public:

        explicit ClipmapBounds(const ClipmapBoundsDescritor& desc);
        ~ClipmapBounds() = default;

        //! Updates the clipmap bounds using a world coordinate center position and returns
        //! 0-2 regions that need to be updated due to moving beyond the margins. These update
        //! regions will always be at least the size of the margin, and will represent horizontal
        //! and/or vertical strips along the edges of the clipmap. An optional untouched region
        //! aabb can be passed to this function to get an aabb of areas inside the bounds of the
        //! clipmap but not updated by the center moving. This can be useful in cases where part
        //! of the bounds of the clipmap is dirty, but areas that will already be updated due
        //! to the center moving shouldn't be updated twice.
        AZStd::vector<ClipmapBoundsRegion> UpdateCenter(const AZ::Vector2& newCenter, AZ::Aabb* untouchedRegion = nullptr);
        
        //! Updates the clipmap bounds using a position in clipmap space (no scaling) and returns
        //! 0-2 regions that need to be updated due to moving beyond the margins. These update
        //! regions will always be at least the size of the margin, and will represent horizontal
        //! and/or vertical strips along the edges of the clipmap. An optional untouched region
        //! aabb can be passed to this function to get an aabb of areas inside the bounds of the
        //! clipmap but not updated by the center moving. This can be useful in cases where part
        //! of the bounds of the clipmap is dirty, but areas that will already be updated due
        //! to the center moving shouldn't be updated twice.
        AZStd::vector<ClipmapBoundsRegion> UpdateCenter(const Vector2i& newCenter, AZ::Aabb* untouchedRegion = nullptr);

        //! Takes in a single world space region and transforms it into 0-4 regions in the clipmap clamped
        //! to the bounds of the clipmap.
        AZStd::vector<ClipmapBoundsRegion> TransformRegion(AZ::Aabb worldSpaceRegion);
        
        //! Takes in a single unscaled clipmap space region and transforms it into 0-4 regions in the clipmap clamped
        //! to the bounds of the clipmap.
        AZStd::vector<ClipmapBoundsRegion> TransformRegion(Aabb2i clipSpaceRegion);

        //! Returns the bounds covered by this clipmap in world space. Z component ignored.
        AZ::Aabb GetWorldBounds();

    private:
        
        //! Returns the bounds covered by the clipmap in local space
        Aabb2i GetLocalBounds() const;

        //! Applies scale and averages a world space vector to get a clip space vector.
        Vector2i GetClipSpaceVector(const AZ::Vector2& worldSpaceVector) const;

        //! Applies inverse scale to get a world aabb from clip space aabb.
        AZ::Aabb GetWorldSpaceAabb(const Aabb2i& clipSpaceAabb) const;

        Vector2i m_center;
        Vector2i m_modCenter;
        int32_t m_size;
        int32_t m_halfSize;
        int32_t m_margin;
        float m_scale;
        float m_rcpScale;

    };

}
