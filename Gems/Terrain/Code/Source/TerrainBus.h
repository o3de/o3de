/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of
 * this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/std/string/string_view.h>
#include <AzCore/Math/Aabb.h>

namespace SurfaceData
{
    struct SurfacePoint;
    using SurfaceTagWeightMap = AZStd::unordered_map<AZ::Crc32, float>;
}

namespace Terrain
{
    class TerrainDataRequests
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        using MutexType = AZStd::recursive_mutex;
        //////////////////////////////////////////////////////////////////////////

        virtual AZ::Vector2 GetTerrainGridResolution() const = 0;
        virtual AZ::Aabb GetTerrainAabb() const = 0;

        virtual float GetHeightSynchronous(float x, float y) = 0;
        virtual AZ::Vector3 GetNormalSynchronous(float x, float y) = 0;

        enum class Sampler
        {
            BILINEAR,   // Get the value at the requested location, using terrain sample grid to bilinear filter between sample grid points
            CLAMP,      // Clamp the input point to the terrain sample grid, then get the exact value
            EXACT,       // Directly get the value at the location, regardless of terrain sample grid density

            DEFAULT = BILINEAR
        };

        virtual void GetHeight(const AZ::Vector3& inPosition, Sampler sampleFilter, AZ::Vector3& outPosition) = 0;
        virtual void GetNormal(const AZ::Vector3& inPosition, Sampler sampleFilter, AZ::Vector3& outNormal) = 0;
        virtual void GetSurfaceWeights(const AZ::Vector3& inPosition, Sampler sampleFilter, SurfaceData::SurfaceTagWeightMap& outSurfaceWeights) = 0;
        virtual void GetSurfacePoint(const AZ::Vector3& inPosition, Sampler sampleFilter, SurfaceData::SurfacePoint& outSurfacePoint) = 0;


        typedef AZStd::function<void()> TerrainDataReadyCallback;
        typedef AZStd::function<void(const SurfaceData::SurfacePoint& surfacePoint, uint32_t xIndex, uint32_t yIndex)> SurfacePointRegionFillCallback;

        virtual void ProcessHeightsFromRegion(const AZ::Aabb& inRegion, const AZ::Vector2 stepSize, Sampler sampleFilter, SurfacePointRegionFillCallback perPositionCallback, TerrainDataReadyCallback onComplete = nullptr) = 0;
        virtual void ProcessSurfacePointsFromRegion(const AZ::Aabb& inRegion, const AZ::Vector2 stepSize, Sampler sampleFilter, SurfacePointRegionFillCallback perPositionCallback, TerrainDataReadyCallback onComplete = nullptr) = 0;

    };
    using TerrainDataRequestBus = AZ::EBus<TerrainDataRequests>;
}

