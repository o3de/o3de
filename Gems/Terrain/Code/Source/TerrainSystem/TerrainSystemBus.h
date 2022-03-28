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
#include <AzCore/std/containers/span.h>
#include <AzCore/std/functional.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>

#include <AzCore/EBus/EBus.h>
#include <AzCore/Component/ComponentBus.h>

#include <AzFramework/Terrain/TerrainDataRequestBus.h>

namespace Terrain
{
    /**
    * A bus to signal the life times of terrain areas
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

        // register an area to override terrain
        virtual void RegisterArea(AZ::EntityId areaId) = 0;
        virtual void UnregisterArea(AZ::EntityId areaId) = 0;
        virtual void RefreshArea(AZ::EntityId areaId, AzFramework::Terrain::TerrainDataNotifications::TerrainDataChangedMask changeMask) = 0;
    };

    using TerrainSystemServiceRequestBus = AZ::EBus<TerrainSystemServiceRequests>;


    /**
    * A bus to signal the life times of terrain areas
    */
    class TerrainAreaHeightRequests
        : public AZ::ComponentBus
    {
    public:
        ////////////////////////////////////////////////////////////////////////
        // EBusTraits
        using MutexType = AZStd::recursive_mutex;

        // This bus will not lock during an EBus call. This lets us run multiple queries in parallel, but it also means
        // that anything that implements this EBus will need to ensure that queries can't be in the middle of running at the
        // same time as bus connects / disconnects.
        static const bool LocklessDispatch = true;
        ////////////////////////////////////////////////////////////////////////

        virtual ~TerrainAreaHeightRequests() = default;

        /// Synchronous single input location.
        /// @inPosition is the input position to query.
        /// @outPosition will have the same XY as inPosition, but with the Z adjusted to the proper height.
        /// @terrainExists is true if the output position is valid terrain.
        virtual void GetHeight(const AZ::Vector3& inPosition, AZ::Vector3& outPosition, bool& terrainExists) = 0;

        /// Synchronous multiple input locations.
        /// @inOutPositionList takes a list of Vector3s as input and returns the Vector3s with Z filled out.
        /// @terrainExistsList outputs flags for whether or not each output position is valid terrain.
        virtual void GetHeights(AZStd::span<AZ::Vector3> inOutPositionList, AZStd::span<bool> terrainExistsList) = 0;
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
        virtual bool GetUseGroundPlane() = 0;

    };

    using TerrainSpawnerRequestBus = AZ::EBus<TerrainSpawnerRequests>;

}
