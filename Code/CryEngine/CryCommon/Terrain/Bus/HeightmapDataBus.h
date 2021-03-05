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

#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/Math/Aabb.h>

namespace Terrain
{
    class Viewport2D
    {
    public:
        int m_topLeftX = 0;
        int m_topLeftY = 0;
        int m_width = 0;
        int m_height = 0;

        Viewport2D() = default;
        Viewport2D(const Viewport2D&) = default;
        Viewport2D& operator=(const Viewport2D&) = default;

        Viewport2D(int topLeftX, int topLeftY, int width, int height)
            : m_topLeftX(topLeftX)
            , m_topLeftY(topLeftY)
            , m_width(width)
            , m_height(height)
        {
        }
    };

    // External height map data requests
    class HeightmapDataRequestInfo
    {
    public:
        HeightmapDataRequestInfo() = default;
        HeightmapDataRequestInfo(const HeightmapDataRequestInfo& rhs) = default;
        HeightmapDataRequestInfo& operator=(const HeightmapDataRequestInfo& rhs) = default;

        HeightmapDataRequestInfo(int viewportTopLeftX, int viewportTopLeftY, int viewportWidth, int viewportHeight, float metersPerPixel, AZ::Vector2 worldMin, AZ::Vector2 worldMax)
            : m_viewport(viewportTopLeftX, viewportTopLeftY, viewportWidth, viewportHeight)
            , m_metersPerPixel(metersPerPixel)
            , m_worldMin(worldMin)
            , m_worldMax(worldMax)
        {
        }

        float GetMetersPerPixel() const
        {
            return m_metersPerPixel;
        }

        AZ::Vector2 GetWorldMin() const
        {
            return m_worldMin;
        }

        AZ::Vector2 GetWorldMax() const
        {
            return m_worldMax;
        }

        AZ::Vector2 GetWorldWidth() const
        {
            return (m_worldMax - m_worldMin);
        }

        Viewport2D GetViewport() const
        {
            return m_viewport;
        }

    private:
        Viewport2D m_viewport;
        AZ::Vector2 m_worldMin = AZ::Vector2(0.0f, 0.0f);
        AZ::Vector2 m_worldMax = AZ::Vector2(0.0f, 0.0f);
        float m_metersPerPixel = 1.0f;
    };

    class HeightmapDataNotifications
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        //////////////////////////////////////////////////////////////////////////

        virtual void OnTerrainHeightDataChanged(const AZ::Aabb& dirtyRegion) = 0;
    };
    using HeightmapDataNotificationBus = AZ::EBus<HeightmapDataNotifications>;
}
