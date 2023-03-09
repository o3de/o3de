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
#include <AzCore/Component/NonUniformScaleBus.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/parallel/shared_mutex.h>
#include <AtomLyIntegration/CommonFeatures/Mesh/MeshComponentBus.h>
#include <SurfaceData/SurfaceDataProviderRequestBus.h>
#include <SurfaceData/SurfaceDataTypes.h>

namespace LmbrCentral
{
    template<typename, typename>
    class EditorWrappedComponentBase;
}

namespace SurfaceData
{
    constexpr float s_rayAABBHeightPadding = 0.1f;

    class SurfaceDataMeshConfig
        : public AZ::ComponentConfig
    {
    public:
        AZ_CLASS_ALLOCATOR(SurfaceDataMeshConfig, AZ::SystemAllocator);
        AZ_RTTI(SurfaceDataMeshConfig, "{764C602E-7CA8-4BCC-AB2D-3E46623B3A20}", AZ::ComponentConfig);
        static void Reflect(AZ::ReflectContext* context);
        SurfaceTagVector m_tags;
    };

    class SurfaceDataMeshComponent
        : public AZ::Component
        , public AZ::TickBus::Handler
        , public AZ::TransformNotificationBus::Handler
        , public AZ::Render::MeshComponentNotificationBus::Handler
        , public SurfaceDataProviderRequestBus::Handler
    {
    public:
        template<typename, typename> friend class LmbrCentral::EditorWrappedComponentBase;
        AZ_COMPONENT(SurfaceDataMeshComponent, "{F8915F34-BE8B-40B4-B7E8-01EBF3DA1C95}");
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void Reflect(AZ::ReflectContext* context);

        SurfaceDataMeshComponent(const SurfaceDataMeshConfig& configuration);
        SurfaceDataMeshComponent();
        ~SurfaceDataMeshComponent() = default;

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Activate() override;
        void Deactivate() override;
        bool ReadInConfig(const AZ::ComponentConfig* baseConfig) override;
        bool WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const override;

        //////////////////////////////////////////////////////////////////////////
        // MeshComponentNotificationBus
        void OnModelReady(const AZ::Data::Asset<AZ::RPI::ModelAsset>& modelAsset, const AZ::Data::Instance<AZ::RPI::Model>& model) override;

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

    private:
        void UpdateMeshData();
        void OnCompositionChanged();

        AZ::Aabb GetSurfaceAabb() const;
        SurfaceTagVector GetSurfaceTags() const;

        AZ::NonUniformScaleChangedEvent::Handler m_nonUniformScaleChangedHandler; ///< Responds to changes in non-uniform scale.

        SurfaceDataMeshConfig m_configuration;

        SurfaceDataRegistryHandle m_providerHandle = InvalidSurfaceDataRegistryHandle;

        // cached data
        AZStd::atomic_bool m_refresh{ false };
        mutable AZStd::shared_mutex m_cacheMutex;
        AZ::Data::Asset<AZ::Data::AssetData> m_meshAssetData;
        AZ::Transform m_meshWorldTM = AZ::Transform::CreateIdentity();
        AZ::Transform m_meshWorldTMInverse = AZ::Transform::CreateIdentity();
        AZ::Vector3 m_meshNonUniformScale = AZ::Vector3::CreateOne();
        AZ::Aabb m_meshBounds = AZ::Aabb::CreateNull();
        SurfaceTagWeights m_newPointWeights;
    };
}
