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

#include <AzFramework/Terrain/TerrainDataRequestBus.h>

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

        // register an area to override terrain
        virtual void RegisterArea(AZ::EntityId areaId) = 0;
        virtual void UnregisterArea(AZ::EntityId areaId) = 0;
        virtual void RefreshArea(AZ::EntityId areaId, AzFramework::Terrain::TerrainDataNotifications::TerrainDataChangedMask changeMask) = 0;
    };

    using TerrainSystemServiceRequestBus = AZ::EBus<TerrainSystemServiceRequests>;


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

        // Synchronous single input location.  The Vector3 input position versions are defined to ignore the input Z value.
        virtual void GetHeight(const AZ::Vector3& inPosition, AZ::Vector3& outPosition, bool& terrainExists) = 0;
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
