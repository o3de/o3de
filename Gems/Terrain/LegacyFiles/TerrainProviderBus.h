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

#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Aabb.h>
#include <AzCore/std/functional.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>

#include <AzCore/EBus/EBus.h>
#include <AzCore/Component/ComponentBus.h>

#include "HeightmapDataBus.h"

class CShader;

namespace Terrain
{
    // This interface defines how the renderer can access the terrain system to set up state and gather information before rendering height maps
    class TerrainProviderRequests
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        //! allows multiple threads to call
        using MutexType = AZStd::recursive_mutex;
        //////////////////////////////////////////////////////////////////////////

        // world properties
        virtual AZ::Aabb GetWorldBounds() = 0;
        virtual AZ::Vector3 GetRegionSize() = 0;

        // utility
        virtual void GetRegionIndex(const AZ::Vector2& worldMin, const AZ::Vector2& worldMax, int& regionIndexX, int& regionIndexY) = 0;

        virtual float GetHeightAtIndexedPosition(int ix, int iy) { return 64.0f; }
        virtual float GetHeightAtWorldPosition(float fx, float fy) { return 64.0f; }
        virtual unsigned char GetSurfaceTypeAtIndexedPosition(int ix, int iy) { return 0; }
    };
    using TerrainProviderRequestBus = AZ::EBus<TerrainProviderRequests>;

    // This class exists for the terrain system to inject data into the renderer for generating the GPU-side terrain height map
    struct CRETerrainContext
    {
        // Tract map
        virtual void OnTractVersionUpdate() = 0;

        CShader* m_currentShader = nullptr;
    };

    class TerrainProviderNotifications
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        //! allows multiple threads to call
        using MutexType = AZStd::recursive_mutex;
        //////////////////////////////////////////////////////////////////////////

        // interface to be implemented by the game, invoked by the terrain render element

        // pull settings from the world cache, so the next accessors are accurate
        virtual void SynchronizeSettings(CRETerrainContext* context) = 0;
    };
    using TerrainProviderNotificationBus = AZ::EBus<TerrainProviderNotifications>;

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

        virtual ~TerrainSystemServiceRequests() AZ_DEFAULT_METHOD;

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

        virtual ~TerrainAreaRequests() AZ_DEFAULT_METHOD;

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

        virtual ~TerrainAreaHeightRequests() AZ_DEFAULT_METHOD;

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

}
