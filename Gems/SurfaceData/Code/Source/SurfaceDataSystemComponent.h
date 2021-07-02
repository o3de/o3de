/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Math/Aabb.h>
#include <SurfaceData/SurfaceDataSystemRequestBus.h>

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
        void GetSurfacePointsFromRegion(const AZ::Aabb& inRegion, const AZ::Vector2 stepSize, const SurfaceTagVector& desiredTags, SurfacePointListPerPosition& surfacePointListPerPosition) const override;

        SurfaceDataRegistryHandle RegisterSurfaceDataProvider(const SurfaceDataRegistryEntry& entry) override;
        void UnregisterSurfaceDataProvider(const SurfaceDataRegistryHandle& handle) override;
        void UpdateSurfaceDataProvider(const SurfaceDataRegistryHandle& handle, const SurfaceDataRegistryEntry& entry) override;

        SurfaceDataRegistryHandle RegisterSurfaceDataModifier(const SurfaceDataRegistryEntry& entry) override;
        void UnregisterSurfaceDataModifier(const SurfaceDataRegistryHandle& handle) override;
        void UpdateSurfaceDataModifier(const SurfaceDataRegistryHandle& handle, const SurfaceDataRegistryEntry& entry) override;

        void RefreshSurfaceData(const AZ::Aabb& dirtyArea) override;
    private:
        void CombineSortAndFilterNeighboringPoints(SurfacePointList& sourcePointList, bool hasDesiredTags, const SurfaceTagVector& desiredTags) const;

        SurfaceDataRegistryHandle RegisterSurfaceDataProviderInternal(const SurfaceDataRegistryEntry& entry);
        SurfaceDataRegistryEntry UnregisterSurfaceDataProviderInternal(const SurfaceDataRegistryHandle& handle);
        bool UpdateSurfaceDataProviderInternal(const SurfaceDataRegistryHandle& handle, const SurfaceDataRegistryEntry& entry, AZ::Aabb& oldBounds);

        SurfaceDataRegistryHandle RegisterSurfaceDataModifierInternal(const SurfaceDataRegistryEntry& entry);
        SurfaceDataRegistryEntry UnregisterSurfaceDataModifierInternal(const SurfaceDataRegistryHandle& handle);
        bool UpdateSurfaceDataModifierInternal(const SurfaceDataRegistryHandle& handle, const SurfaceDataRegistryEntry& entry, AZ::Aabb& oldBounds);

        mutable AZStd::recursive_mutex m_registrationMutex;
        AZStd::unordered_map<SurfaceDataRegistryHandle, SurfaceDataRegistryEntry> m_registeredSurfaceDataProviders;
        AZStd::unordered_map<SurfaceDataRegistryHandle, SurfaceDataRegistryEntry> m_registeredSurfaceDataModifiers;
        SurfaceDataRegistryHandle m_registeredSurfaceDataProviderHandleCounter = InvalidSurfaceDataRegistryHandle;
        SurfaceDataRegistryHandle m_registeredSurfaceDataModifierHandleCounter = InvalidSurfaceDataRegistryHandle;
        AZStd::unordered_set<AZ::u32> m_registeredModifierTags;

        //point vector reserved for reuse
        mutable SurfacePointList m_targetPointList;
    };
}
