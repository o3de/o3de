/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Aabb.h>
#include <AzCore/std/functional.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>

#include <AzCore/EBus/EBus.h>
#include <AzCore/Component/ComponentBus.h>

namespace Terrain
{
    /**
    * A bus to signal the life times of terrain areas
    * Note: all the API are meant to be queued events
    */
    class TerrainSystemServiceRequests
        : public AZ::EBusTraits
    {
    public:
        ////////////////////////////////////////////////////////////////////////
        // EBusTraits
        // singleton pattern 
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        using MutexType = AZStd::recursive_mutex;
        ////////////////////////////////////////////////////////////////////////

        virtual ~TerrainSystemServiceRequests() = default;

        virtual void Activate() = 0;
        virtual void Deactivate() = 0;

        virtual void SetWorldMin(AZ::Vector3 worldOrigin) = 0;
        virtual void SetWorldMax(AZ::Vector3 worldBounds) = 0;
        virtual void SetHeightQueryResolution(AZ::Vector2 queryResolution) = 0;
        virtual void SetDebugWireframe(bool wireframeEnabled) = 0;

        // register an area to override terrain
        virtual void RegisterArea(AZ::EntityId areaId) = 0;
        virtual void UnregisterArea(AZ::EntityId areaId) = 0;
        virtual void RefreshArea(AZ::EntityId areaId) = 0;
    };

    using TerrainSystemServiceRequestBus = AZ::EBus<TerrainSystemServiceRequests>;

    /**
    * A bus to signal the life times of terrain areas
    * Note: all the API are meant to be queued events
    */
    class TerrainAreaRequests
        : public AZ::ComponentBus
    {
    public:
        ////////////////////////////////////////////////////////////////////////
        // EBusTraits
        using MutexType = AZStd::recursive_mutex;
        ////////////////////////////////////////////////////////////////////////

        virtual ~TerrainAreaRequests() = default;

        virtual void RegisterArea() = 0;
        virtual void RefreshArea() = 0;

    };

    using TerrainAreaRequestBus = AZ::EBus<TerrainAreaRequests>;

    /**
    * A bus to signal the life times of terrain areas
    * Note: all the API are meant to be queued events
    */
    class TerrainAreaHeightRequests
        : public AZ::ComponentBus
    {
    public:
        ////////////////////////////////////////////////////////////////////////
        // EBusTraits
        using MutexType = AZStd::recursive_mutex;
        ////////////////////////////////////////////////////////////////////////

        virtual ~TerrainAreaHeightRequests() = default;

        enum class Sampler
        {
            BILINEAR,   // Get the value at the requested location, using terrain sample grid to bilinear filter between sample grid points
            CLAMP,      // Clamp the input point to the terrain sample grid, then get the exact value
            EXACT,       // Directly get the value at the location, regardless of terrain sample grid density

            DEFAULT = BILINEAR
        };

        enum SurfacePointDataMask
        {
            POSITION = 0x01,
            NORMAL = 0x02,
            SURFACE_WEIGHTS = 0x04,

            DEFAULT = POSITION | NORMAL | SURFACE_WEIGHTS
        };

        // Synchronous single input location.  The Vector3 input position versions are defined to ignore the input Z value.

        virtual void GetHeight(const AZ::Vector3& inPosition, AZ::Vector3& outPosition, Sampler sampleFilter = Sampler::DEFAULT) = 0;
        virtual void GetNormal(const AZ::Vector3& inPosition, AZ::Vector3& outNormal, Sampler sampleFilter = Sampler::DEFAULT) = 0;
        //virtual void GetSurfaceWeights(const AZ::Vector3& inPosition, SurfaceTagWeightMap& outSurfaceWeights, Sampler sampleFilter = DEFAULT) = 0;
        //virtual void GetSurfacePoint(const AZ::Vector3& inPosition, SurfacePoint& outSurfacePoint, SurfacePointDataMask dataMask = DEFAULT, Sampler sampleFilter = DEFAULT) = 0;
    };

    using TerrainAreaHeightRequestBus = AZ::EBus<TerrainAreaHeightRequests>;
    
    /**
    * A bus for the TerrainSystem to interrogate TerrainLayerSpawners.
    */
    class TerrainSpawnerRequests
        : public AZ::ComponentBus
    {
    public:
        ////////////////////////////////////////////////////////////////////////
        // EBusTraits
        using MutexType = AZStd::recursive_mutex;
        ////////////////////////////////////////////////////////////////////////

        virtual ~TerrainSpawnerRequests() = default;

        virtual void GetPriority(AZ::u32& outLayer, AZ::u32& outPriority) = 0;
        virtual void GetUseGroundPlane(bool& outUseGroundPlane) = 0;

    };

    using TerrainSpawnerRequestBus = AZ::EBus<TerrainSpawnerRequests>;

}
