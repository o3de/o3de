/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Math/Aabb.h>
#include <AzCore/std/parallel/shared_mutex.h>
#include <SurfaceData/SurfaceDataSystemRequestBus.h>
#include <SurfaceData/SurfaceDataTypes.h>

namespace SurfaceData
{
    class SurfaceDataSystemComponent
        : public AZ::Component
        , private SurfaceDataSystemRequestBus::Handler
    {
    public:
        AZ_COMPONENT(SurfaceDataSystemComponent, "{6F334BAA-7BD5-45F8-A9BA-760667D25FA0}");

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

    protected:
        ////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        
        
        ////////////////////////////////////////////////////////////////////////
        // SurfaceDataSystemRequestBus implementation
        void GetSurfacePoints(const AZ::Vector3& inPosition, const SurfaceTagVector& desiredTags, SurfacePointList& surfacePointList) const override;
        void GetSurfacePointsFromRegion(
            const AZ::Aabb& inRegion, const AZ::Vector2 stepSize, const SurfaceTagVector& desiredTags,
            SurfacePointList& surfacePointListPerPosition) const override;
        void GetSurfacePointsFromList(
            AZStd::span<const AZ::Vector3> inPositions,
            const SurfaceTagVector& desiredTags,
            SurfacePointList& surfacePointLists) const override;

        void GetSurfacePointsFromListInternal(
            AZStd::span<const AZ::Vector3> inPositions,
            const AZ::Aabb& inBounds,
            const SurfaceTagVector& desiredTags,
            SurfacePointList& surfacePointLists) const;

        SurfaceDataRegistryHandle RegisterSurfaceDataProvider(const SurfaceDataRegistryEntry& entry) override;
        void UnregisterSurfaceDataProvider(const SurfaceDataRegistryHandle& handle) override;
        void UpdateSurfaceDataProvider(const SurfaceDataRegistryHandle& handle, const SurfaceDataRegistryEntry& entry) override;

        SurfaceDataRegistryHandle RegisterSurfaceDataModifier(const SurfaceDataRegistryEntry& entry) override;
        void UnregisterSurfaceDataModifier(const SurfaceDataRegistryHandle& handle) override;
        void UpdateSurfaceDataModifier(const SurfaceDataRegistryHandle& handle, const SurfaceDataRegistryEntry& entry) override;

        void RefreshSurfaceData(const SurfaceDataRegistryHandle& providerHandle, const AZ::Aabb& dirtyArea) override;

        SurfaceDataRegistryHandle GetSurfaceDataProviderHandle(const AZ::EntityId& providerEntityId) override;
        SurfaceDataRegistryHandle GetSurfaceDataModifierHandle(const AZ::EntityId& modifierEntityId) override;

    private:

        using SurfaceDataRegistryMap = AZStd::unordered_map<SurfaceDataRegistryHandle, SurfaceDataRegistryEntry>;

        // Get all the surface tags that can exist within the given bounds.
        SurfaceTagSet GetTagsFromBounds(const AZ::Aabb& bounds, const SurfaceDataRegistryMap& registeredEntries) const;
        // Get all the surface provider tags that can exist within the given bounds.
        SurfaceTagSet GetProviderTagsFromBounds(const AZ::Aabb& bounds) const;
        // Get all the surface modifier tags that can exist within the given bounds.
        SurfaceTagSet GetModifierTagsFromBounds(const AZ::Aabb& bounds) const;

        // Get all of the surface tags that can be affected by surface provider changes within the given bounds.
        SurfaceTagSet GetAffectedSurfaceTags(const AZ::Aabb& bounds, const SurfaceTagVector& providerTags) const;

        // Convert a SurfaceTagVector to a SurfaceTagSet.
        SurfaceTagSet ConvertTagVectorToSet(const SurfaceTagVector& surfaceTags) const;

        SurfaceDataRegistryHandle RegisterSurfaceDataProviderInternal(const SurfaceDataRegistryEntry& entry);
        SurfaceDataRegistryEntry UnregisterSurfaceDataProviderInternal(const SurfaceDataRegistryHandle& handle);
        bool UpdateSurfaceDataProviderInternal(const SurfaceDataRegistryHandle& handle, const SurfaceDataRegistryEntry& entry, AZ::Aabb& oldBounds);

        SurfaceDataRegistryHandle RegisterSurfaceDataModifierInternal(const SurfaceDataRegistryEntry& entry);
        SurfaceDataRegistryEntry UnregisterSurfaceDataModifierInternal(const SurfaceDataRegistryHandle& handle);
        bool UpdateSurfaceDataModifierInternal(const SurfaceDataRegistryHandle& handle, const SurfaceDataRegistryEntry& entry, AZ::Aabb& oldBounds);

        mutable AZStd::shared_mutex m_registrationMutex;
        SurfaceDataRegistryMap m_registeredSurfaceDataProviders;
        SurfaceDataRegistryMap m_registeredSurfaceDataModifiers;
        SurfaceDataRegistryHandle m_registeredSurfaceDataProviderHandleCounter = InvalidSurfaceDataRegistryHandle;
        SurfaceDataRegistryHandle m_registeredSurfaceDataModifierHandleCounter = InvalidSurfaceDataRegistryHandle;
        AZStd::unordered_set<AZ::u32> m_registeredModifierTags;

        //point vector reserved for reuse
        mutable SurfacePointList m_targetPointList;
    };
}
