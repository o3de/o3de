/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Entity.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/Math/Aabb.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/std/containers/span.h>
#include <SurfaceData/SurfaceDataTypes.h>
#include <SurfaceData/SurfacePointList.h>

namespace SurfaceData
{
    class SurfaceDataSystem
    {
    public:
        AZ_RTTI(SurfaceDataSystem, "{381E1C98-F942-434D-B0C7-22F1AFB679A9}");

        // Get all surface points located at the inPosition that matches one or more of the desiredTags.  Only the XY components of
        // inPosition are used.
        virtual void GetSurfacePoints(
            const AZ::Vector3& inPosition, const SurfaceTagVector& desiredTags, SurfacePointList& surfacePointList) const = 0;

        // Get all surface points for every input position within an AABB region.  Only the XY dimensions of the AABB region are used.
        // The input positions are chosen by starting at the min sides of inRegion and incrementing by stepSize.  This method is inclusive
        // on the min sides of the AABB, and exclusive on the max sides (i.e. for a box of (0,0) - (4,4), the point (0,0) is included but
        // (4,4) isn't).
        virtual void GetSurfacePointsFromRegion(
            const AZ::Aabb& inRegion,
            const AZ::Vector2 stepSize,
            const SurfaceTagVector& desiredTags,
            SurfacePointList& surfacePointLists) const = 0;

        // Get all surface points for every passed-in input position.  Only the XY dimensions of each position are used.
        virtual void GetSurfacePointsFromList(
            AZStd::span<const AZ::Vector3> inPositions, const SurfaceTagVector& desiredTags, SurfacePointList& surfacePointLists) const = 0;

        virtual SurfaceDataRegistryHandle RegisterSurfaceDataProvider(const SurfaceDataRegistryEntry& entry) = 0;
        virtual void UnregisterSurfaceDataProvider(const SurfaceDataRegistryHandle& handle) = 0;
        virtual void UpdateSurfaceDataProvider(const SurfaceDataRegistryHandle& handle, const SurfaceDataRegistryEntry& entry) = 0;

        virtual SurfaceDataRegistryHandle RegisterSurfaceDataModifier(const SurfaceDataRegistryEntry& entry) = 0;
        virtual void UnregisterSurfaceDataModifier(const SurfaceDataRegistryHandle& handle) = 0;
        virtual void UpdateSurfaceDataModifier(const SurfaceDataRegistryHandle& handle, const SurfaceDataRegistryEntry& entry) = 0;

        // Notify any dependent systems that they need to refresh their surface data for the provided area.
        virtual void RefreshSurfaceData(const AZ::Aabb& dirtyArea) = 0;

        // Get the SurfaceDataRegistryHandle for a given entityId.
        virtual SurfaceDataRegistryHandle GetSurfaceDataProviderHandle(const AZ::EntityId& providerEntityId) = 0;
        virtual SurfaceDataRegistryHandle GetSurfaceDataModifierHandle(const AZ::EntityId& modifierEntityId) = 0;
    };

    /**
    * the EBus is used to request information about a surface
    */
    class SurfaceDataSystemRequestTraits
        : public AZ::EBusTraits
    {
    public:
        ////////////////////////////////////////////////////////////////////////
        // EBusTraits
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        ////////////////////////////////////////////////////////////////////////
    };

    using SurfaceDataSystemRequestBus = AZ::EBus<SurfaceDataSystem, SurfaceDataSystemRequestTraits>;
}
