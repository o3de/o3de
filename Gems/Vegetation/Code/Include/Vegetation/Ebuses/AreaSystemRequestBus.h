/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/Math/Aabb.h>

namespace Vegetation
{
    struct InstanceData;

    //determines if enumeration should proceed
    enum class AreaSystemEnumerateCallbackResult : AZ::u8
    {
        StopEnumerating = 0,
        KeepEnumerating,
    };
    using AreaSystemEnumerateCallback = AZStd::function<AreaSystemEnumerateCallbackResult(const InstanceData&)>;

    /**
    * A bus to signal the life times of vegetation areas
    * Note: all the API are meant to be queued events
    */
    class AreaSystemRequests
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

        virtual ~AreaSystemRequests() = default;

        // register an area to override vegetation; returns a handle to used to unregister the area
        virtual void RegisterArea(AZ::EntityId areaId, AZ::u32 layer, AZ::u32 priority, const AZ::Aabb& bounds) = 0;
        virtual void UnregisterArea(AZ::EntityId areaId) = 0;
        virtual void RefreshArea(AZ::EntityId areaId, AZ::u32 layer, AZ::u32 priority, const AZ::Aabb& bounds) = 0;
        virtual void RefreshAllAreas() = 0;
        virtual void ClearAllAreas() = 0;

        // to allow areas to be combined into an area blender
        virtual void MuteArea(AZ::EntityId areaId) = 0;
        virtual void UnmuteArea(AZ::EntityId areaId) = 0;

        //! Visit all instances contained within every vegetation sector that overlaps the given bounds
        //! until callback decides otherwise.  The sector boundary is additionally expanded by the sector
        //! search padding value in the Area System component's configuration.
        //! @param bounds The AABB that contains the bounds of the area to expand and scan for instances
        //! @param callback The function to call for every instance found
        virtual void EnumerateInstancesInOverlappingSectors(const AZ::Aabb& bounds, AreaSystemEnumerateCallback callback) const = 0;

        //! Visit all instances contained within bounds until callback decides otherwise.
        //! @param bounds The AABB that contains the area to scan for instances
        //! @param callback The function to call for every instance found
        virtual void EnumerateInstancesInAabb(const AZ::Aabb& bounds, AreaSystemEnumerateCallback callback) const = 0;

        // Get the current number of instances contained within the AABB
        virtual AZStd::size_t GetInstanceCountInAabb(const AZ::Aabb& bounds) const = 0;

        //! Get the list of instances contained within the AABB
        //! @param bounds The AABB that contains the area to scan for instances
        //! @return The returned list of instances
        virtual AZStd::vector<Vegetation::InstanceData> GetInstancesInAabb(const AZ::Aabb& bounds) const = 0;
    };

    using AreaSystemRequestBus = AZ::EBus<AreaSystemRequests>;
} // namespace Vegetation

