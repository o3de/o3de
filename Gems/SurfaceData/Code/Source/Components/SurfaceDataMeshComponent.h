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

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/std/containers/unordered_map.h>
#include <LmbrCentral/Rendering/MeshComponentBus.h>
#include <SurfaceData/SurfaceDataProviderRequestBus.h>
#include <SurfaceData/SurfaceDataTypes.h>

namespace LmbrCentral
{
    class MeshAsset;

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
        AZ_CLASS_ALLOCATOR(SurfaceDataMeshConfig, AZ::SystemAllocator, 0);
        AZ_RTTI(SurfaceDataMeshConfig, "{764C602E-7CA8-4BCC-AB2D-3E46623B3A20}", AZ::ComponentConfig);
        static void Reflect(AZ::ReflectContext* context);
        SurfaceTagVector m_tags;
    };

    class SurfaceDataMeshComponent
        : public AZ::Component
        , public AZ::TickBus::Handler
        , public AZ::TransformNotificationBus::Handler
        , public LmbrCentral::MeshComponentNotificationBus::Handler
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
        SurfaceDataMeshComponent() = default;
        ~SurfaceDataMeshComponent() = default;

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Activate() override;
        void Deactivate() override;
        bool ReadInConfig(const AZ::ComponentConfig* baseConfig) override;
        bool WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const override;

        //////////////////////////////////////////////////////////////////////////
        // MeshComponentNotificationBus
        void OnMeshCreated(const AZ::Data::Asset<AZ::Data::AssetData>& asset) override;
        void OnMeshDestroyed() override;
        void OnBoundsReset() override;;

        //////////////////////////////////////////////////////////////////////////
        // TransformNotificationBus
        void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;

        ////////////////////////////////////////////////////////////////////////
        // AZ::TickBus
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;

        ////////////////////////////////////////////////////////////////////////
        // SurfaceDataProviderRequestBus
        void GetSurfacePoints(const AZ::Vector3& inPosition, SurfacePointList& surfacePointList) const override;

    private:
        bool DoRayTrace(const AZ::Vector3& inPosition, AZ::Vector3& outPosition, AZ::Vector3& outNormal) const;
        void UpdateMeshData();
        void OnCompositionChanged();

        AZ::Aabb GetSurfaceAabb() const;
        SurfaceTagVector GetSurfaceTags() const;

        SurfaceDataMeshConfig m_configuration;

        SurfaceDataRegistryHandle m_providerHandle = InvalidSurfaceDataRegistryHandle;

        // cached data
        AZStd::atomic_bool m_refresh{ false };
        mutable AZStd::recursive_mutex m_cacheMutex;
        AZ::Data::Asset<AZ::Data::AssetData> m_meshAssetData;
        AZ::Transform m_meshWorldTM = AZ::Transform::CreateIdentity();
        AZ::Transform m_meshWorldTMInverse = AZ::Transform::CreateIdentity();
        AZ::Aabb m_meshBounds = AZ::Aabb::CreateNull();
    };
}