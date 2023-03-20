/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/std/parallel/shared_mutex.h>
#include <AzFramework/Physics/ColliderComponentBus.h>

#include <SurfaceData/SurfaceDataTypes.h>
#include <SurfaceData/SurfaceDataProviderRequestBus.h>
#include <SurfaceData/SurfaceDataModifierRequestBus.h>

namespace LmbrCentral
{
    template<typename, typename>
    class EditorWrappedComponentBase;
}

namespace SurfaceData
{
    class SurfaceDataColliderConfig
        : public AZ::ComponentConfig
    {
    public:
        AZ_CLASS_ALLOCATOR(SurfaceDataColliderConfig, AZ::SystemAllocator);
        AZ_RTTI(SurfaceDataColliderConfig, "{D435DDB9-C513-4A2E-B0AC-9933E9360857}", AZ::ComponentConfig);
        static void Reflect(AZ::ReflectContext* context);
        SurfaceTagVector m_providerTags;
        SurfaceTagVector m_modifierTags;
    };

    class SurfaceDataColliderComponent
        : public AZ::Component
        , public AZ::TickBus::Handler
        , public AZ::TransformNotificationBus::Handler
        , public SurfaceDataProviderRequestBus::Handler
        , private SurfaceDataModifierRequestBus::Handler
        , public Physics::ColliderComponentEventBus::Handler
    {
    public:
        template<typename, typename> friend class LmbrCentral::EditorWrappedComponentBase;
        AZ_COMPONENT(SurfaceDataColliderComponent, "{8BECC930-9B2A-442D-A291-8A3F6B6D1071}");
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void Reflect(AZ::ReflectContext* context);

        SurfaceDataColliderComponent(const SurfaceDataColliderConfig& configuration);
        SurfaceDataColliderComponent() = default;
        ~SurfaceDataColliderComponent() = default;

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Activate() override;
        void Deactivate() override;
        bool ReadInConfig(const AZ::ComponentConfig* baseConfig) override;
        bool WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const override;

        //////////////////////////////////////////////////////////////////////////
        // ColliderComponentEventBus
        // For physics meshes only
        void OnColliderChanged() override;

        //////////////////////////////////////////////////////////////////////////
        // TransformNotificationBus
        void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;

        ////////////////////////////////////////////////////////////////////////
        // AZ::TickBus
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;

        ////////////////////////////////////////////////////////////////////////
        // SurfaceDataProviderRequestBus
        void GetSurfacePoints(const AZ::Vector3& inPosition, SurfacePointList& surfacePointList) const override;
        void GetSurfacePointsFromList(AZStd::span<const AZ::Vector3> inPositions, SurfacePointList& surfacePointList) const override;

        //////////////////////////////////////////////////////////////////////////
        // SurfaceDataModifierRequestBus
        void ModifySurfacePoints(
            AZStd::span<const AZ::Vector3> positions,
            AZStd::span<const AZ::EntityId> creatorEntityIds,
            AZStd::span<SurfaceData::SurfaceTagWeights> weights) const override;

    private:
        void UpdateColliderData();
        void OnCompositionChanged();

        SurfaceDataColliderConfig m_configuration;

        SurfaceDataRegistryHandle m_providerHandle = InvalidSurfaceDataRegistryHandle;
        SurfaceDataRegistryHandle m_modifierHandle = InvalidSurfaceDataRegistryHandle;

        // cached data
        AZStd::atomic_bool m_refresh{ false };
        mutable AZStd::shared_mutex m_cacheMutex;
        AZ::Aabb m_colliderBounds = AZ::Aabb::CreateNull();
        SurfaceTagWeights m_newPointWeights;
    };
}
