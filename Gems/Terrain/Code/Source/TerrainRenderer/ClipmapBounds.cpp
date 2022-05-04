/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TerrainRenderer/ClipmapBounds.h>
#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/Math/MathUtils.h>

namespace Terrain
{
    bool ClipmapBoundsRegion::operator==(const ClipmapBoundsRegion& other) const
    {
        return m_localAabb == other.m_localAabb && m_worldAabb.IsClose(other.m_worldAabb);
    }
    
    bool ClipmapBoundsRegion::operator!=(const ClipmapBoundsRegion& other) const
    {
        return !(*this == other);
    }

    ClipmapBounds::ClipmapBounds(const ClipmapBoundsDescriptor& desc)
        : m_size(desc.m_size)
        , m_halfSize(desc.m_size >> 1)
        , m_clipmapUpdateMultiple(AZ::GetMax<uint32_t>(desc.m_clipmapUpdateMultiple, 1))
        , m_scale(desc.m_clipToWorldScale)
        , m_rcpScale(1.0f / desc.m_clipToWorldScale)
    {
        AZ_Error("ClipmapBounds", m_scale > 0.0f, "ClipmapBounds should have a scale that is greater than 0.0f.");
        m_scale = AZ::GetMax(m_scale, AZ::Constants::FloatEpsilon);

        // recalculate m_center
        m_center = GetSnappedCenter(GetClipSpaceVector(desc.m_worldSpaceCenter));
    }
    
    auto ClipmapBounds::UpdateCenter(const AZ::Vector2& newCenter, AZ::Aabb* untouchedRegion) -> ClipmapBoundsRegionList
    {
        return UpdateCenter(GetClipSpaceVector(newCenter), untouchedRegion);
    }

    auto ClipmapBounds::UpdateCenter(const Vector2i& newCenter, AZ::Aabb* untouchedRegion) -> ClipmapBoundsRegionList
    {
        AZStd::vector<Aabb2i> updateRegions;

        // If the new snapped center isn't the same as the old, then generate update regions in clipmap space
        Vector2i updatedCenter = GetSnappedCenter(newCenter);

        int32_t xDiff = updatedCenter.m_x - m_center.m_x;
        int32_t updateWidth = AZStd::GetMin<uint32_t>(abs(xDiff), m_size);

        /*
        Calculate the update regions. In the common case, there will be two update regions that form either
        an L or inverted L shape. To avoid double-counting the corner, it is always put in the vertical box:
         _
        | | 
        | |____
        |_|____|

        */

        // Calculate the vertical box
        if (updatedCenter.m_x != m_center.m_x)
        {
            updateRegions.push_back();
            Aabb2i& updateRegion = updateRegions.back();
            
            if (updatedCenter.m_x < m_center.m_x)
            {
                updateRegion.m_min.m_x = updatedCenter.m_x - m_halfSize;
                updateRegion.m_max.m_x = updateRegion.m_min.m_x + updateWidth;
            }
            else
            {
                updateRegion.m_max.m_x = updatedCenter.m_x + m_halfSize;
                updateRegion.m_min.m_x = updateRegion.m_max.m_x - updateWidth;
            }
            updateRegion.m_min.m_y = updatedCenter.m_y - m_halfSize;
            updateRegion.m_max.m_y = updatedCenter.m_y + m_halfSize;
        }

        // Calculate the horizontal box
        if (updatedCenter.m_y != m_center.m_y && updateWidth < m_size)
        {
            updateRegions.push_back();
            Aabb2i& updateRegion = updateRegions.back();
            
            uint32_t updateHeight = AZStd::GetMin<uint32_t>(abs(updatedCenter.m_y - m_center.m_y), m_size);
            if (updatedCenter.m_y < m_center.m_y)
            {
                updateRegion.m_min.m_y = updatedCenter.m_y - m_halfSize;
                updateRegion.m_max.m_y = updateRegion.m_min.m_y + updateHeight;
            }
            else
            {
                updateRegion.m_max.m_y = updatedCenter.m_y + m_halfSize;
                updateRegion.m_min.m_y = updateRegion.m_max.m_y - updateHeight;
            }

            // If there was a vertical box, then don't double-count the corner of the update.
            if (xDiff < 0)
            {
                updateRegion.m_min.m_x = updatedCenter.m_x - m_halfSize + updateWidth;
                updateRegion.m_max.m_x = updatedCenter.m_x + m_halfSize;
            }
            else if (xDiff > 0)
            {
                updateRegion.m_min.m_x = updatedCenter.m_x - m_halfSize;
                updateRegion.m_max.m_x = updatedCenter.m_x + m_halfSize - updateWidth;
            }
        }

        if (untouchedRegion)
        {
            // Default to the entire area being untouched.
            AZ::Aabb worldBounds = GetWorldBounds();
            float maxX = worldBounds.GetMax().GetX();
            float minX = worldBounds.GetMin().GetX();
            float maxY = worldBounds.GetMax().GetY();
            float minY = worldBounds.GetMin().GetY();

            if (updatedCenter.m_x < m_center.m_x)
            {
                maxX = (updatedCenter.m_x + m_halfSize) * m_rcpScale;
            }
            else if (updatedCenter.m_x > m_center.m_x)
            {
                minX = (updatedCenter.m_x - m_halfSize) * m_rcpScale;
            }
            if (updatedCenter.m_y < m_center.m_y)
            {
                maxY = (updatedCenter.m_y + m_halfSize) * m_rcpScale;
            }
            else if (updatedCenter.m_y > m_center.m_y)
            {
                minY = (updatedCenter.m_y - m_halfSize) * m_rcpScale;
            }

            untouchedRegion->Set(AZ::Vector3(minX, minY, 0.0f), AZ::Vector3(maxX, maxY, 0.0f));
        }

        m_center = updatedCenter;
        
        m_modCenter.m_x = (m_size + (m_center.m_x % m_size)) % m_size;
        m_modCenter.m_y = (m_size + (m_center.m_y % m_size)) % m_size;

        ClipmapBoundsRegionList boundsUpdate;
        for (Aabb2i& updateRegion : updateRegions)
        {
            ClipmapBoundsRegionList update = TransformRegion(updateRegion);
            boundsUpdate.insert(boundsUpdate.end(), update.begin(), update.end());
        }

        return boundsUpdate;
    }
    
    auto ClipmapBounds::TransformRegion(AZ::Aabb worldSpaceRegion) -> ClipmapBoundsRegionList
    {
        AZ::Vector2 worldMin = AZ::Vector2(worldSpaceRegion.GetMin().GetX(), worldSpaceRegion.GetMin().GetY());
        AZ::Vector2 worldMax = AZ::Vector2(worldSpaceRegion.GetMax().GetX(), worldSpaceRegion.GetMax().GetY());

        Aabb2i clipSpaceRegion;
        clipSpaceRegion.m_min = GetClipSpaceVector(worldMin);
        clipSpaceRegion.m_max = GetClipSpaceVector(worldMax);

        return TransformRegion(clipSpaceRegion);
    }

    auto ClipmapBounds::TransformRegion(Aabb2i region) -> ClipmapBoundsRegionList
    {
        ClipmapBoundsRegionList transformedRegions;

        Aabb2i clampedRegion = region.GetClamped(GetLocalBounds());
        if (!clampedRegion.IsValid())
        {
            // Early out if the region is outside the bounds
            return transformedRegions;
        }

        Vector2i minCorner = m_center - m_halfSize;

        Vector2i minBoundary;
        minBoundary.m_x = (minCorner.m_x / m_size - (minCorner.m_x < 0 ? 1 : 0)) * m_size;
        minBoundary.m_y = (minCorner.m_y / m_size - (minCorner.m_y < 0 ? 1 : 0)) * m_size;

        Aabb2i bottomLeftTile = Aabb2i(minBoundary, minBoundary + m_size);
        
        // For each of the 4 quadrants:
        auto calculateQuadrant = [&](Aabb2i tile)
        {
            Aabb2i regionClampedToTile = clampedRegion.GetClamped(tile);
            if (regionClampedToTile.IsValid())
            {
                transformedRegions.push_back(
                    ClipmapBoundsRegion({
                        GetWorldSpaceAabb(regionClampedToTile),
                        regionClampedToTile - tile.m_min
                    })
                );
            }
        };

        calculateQuadrant(bottomLeftTile);
        calculateQuadrant(bottomLeftTile + Vector2i(m_size, 0));
        calculateQuadrant(bottomLeftTile + Vector2i(0, m_size));
        calculateQuadrant(bottomLeftTile + Vector2i(m_size, m_size));
        
        return transformedRegions;
    }

    AZ::Aabb ClipmapBounds::GetWorldBounds() const
    {
        Aabb2i localBounds = GetLocalBounds();
        
        return AZ::Aabb::CreateFromMinMaxValues(
            localBounds.m_min.m_x * m_scale, localBounds.m_min.m_y * m_scale, 0.0f,
            localBounds.m_max.m_x * m_scale, localBounds.m_max.m_y * m_scale, 0.0f);
    }

    float ClipmapBounds::GetWorldSpaceSafeDistance() const
    {
        return (m_halfSize - m_clipmapUpdateMultiple) * m_scale;
    }

    Vector2i ClipmapBounds::GetSnappedCenter(const Vector2i& center)
    {
        Vector2i updatedCenter = m_center;

        // Update the snapped center if the new center has drifted beyond the margin
        auto UpdateDim = [&](int32_t centerDim, int32_t& snappedCenterDim) -> void
        {
            int32_t diff = centerDim - snappedCenterDim;
            
            int32_t scaledCenterDim = (centerDim / m_clipmapUpdateMultiple);
            if (centerDim < 0)
            {
                // Force rounding down for negatives
                scaledCenterDim--;
            }

            if (diff >= m_clipmapUpdateMultiple)
            {
                snappedCenterDim = scaledCenterDim * m_clipmapUpdateMultiple;
            }
            if (diff < -m_clipmapUpdateMultiple)
            {
                snappedCenterDim = (scaledCenterDim + 1) * m_clipmapUpdateMultiple;
            }
        };
        UpdateDim(center.m_x, updatedCenter.m_x);
        UpdateDim(center.m_y, updatedCenter.m_y);

        return updatedCenter;
    }

    Aabb2i ClipmapBounds::GetLocalBounds() const
    {
        return Aabb2i(m_center - m_halfSize, m_center + m_halfSize);
    }
    
    Vector2i ClipmapBounds::GetClipSpaceVector(const AZ::Vector2& worldSpaceVector) const
    {
        // Get rounded integer x/y coords in clipmap space.
        int32_t x = AZStd::lround(worldSpaceVector.GetX() * m_rcpScale);
        int32_t y = AZStd::lround(worldSpaceVector.GetY() * m_rcpScale);
        return Vector2i(x, y);
    }

    AZ::Aabb ClipmapBounds::GetWorldSpaceAabb(const Aabb2i& clipSpaceAabb) const
    {
        return AZ::Aabb::CreateFromMinMaxValues(
            clipSpaceAabb.m_min.m_x * m_scale, clipSpaceAabb.m_min.m_y * m_scale, 0.0f,
            clipSpaceAabb.m_max.m_x * m_scale, clipSpaceAabb.m_max.m_y * m_scale, 0.0f
        );
    }

    AZ::Vector2 ClipmapBounds::GetNormalizedCenter() const
    {
        AZ::Vector2 normalizedCenter(aznumeric_cast<float>(m_modCenter.m_x) + 0.5f, aznumeric_cast<float>(m_modCenter.m_y) + 0.5f);
        normalizedCenter /= aznumeric_cast<float>(m_size);
        return normalizedCenter;
    }

}
