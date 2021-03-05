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
#include <Vegetation/Ebuses/AreaRequestBus.h>
#include <Vegetation/Ebuses/MeshBlockerRequestBus.h>
#include <LmbrCentral/Dependency/DependencyNotificationBus.h>
#include <LmbrCentral/Rendering/MeshComponentBus.h>
#include <SurfaceData/SurfaceDataSystemNotificationBus.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/std/containers/unordered_map.h>
#include <Vegetation/Ebuses/AreaNotificationBus.h>
#include <Vegetation/AreaComponentBase.h>

namespace LmbrCentral
{
    class MeshAsset; 
    
    template<typename, typename>
    class EditorWrappedComponentBase;
}

namespace Vegetation
{
    class MeshBlockerConfig
        : public AreaConfig
    {
    public:
        AZ_CLASS_ALLOCATOR(MeshBlockerConfig, AZ::SystemAllocator, 0);
        AZ_RTTI(MeshBlockerConfig, "{1D00F234-8134-4A42-A357-ADAC865CF63A}", AreaConfig);
        static void Reflect(AZ::ReflectContext* context);

        MeshBlockerConfig() : AreaConfig() { m_priority = AreaConstants::s_priorityMax; m_layer = AreaConstants::s_foregroundLayer; }
        bool m_inheritBehavior = true;
        float m_meshHeightPercentMin = 0.0f;
        float m_meshHeightPercentMax = 1.0f;
        bool m_blockWhenInvisible = true;
    };

    static const AZ::Uuid MeshBlockerComponentTypeId = "{06A1ABB3-F2CD-47FC-BDE3-A13E37F3D760}";

    class MeshBlockerComponent
        : public AreaComponentBase
        , private LmbrCentral::MeshComponentNotificationBus::Handler
        , private AZ::TickBus::Handler
        , private MeshBlockerRequestBus::Handler
        , private SurfaceData::SurfaceDataSystemNotificationBus::Handler
    {
    public:
        friend class EditorMeshBlockerComponent;
        template<typename, typename> friend class LmbrCentral::EditorWrappedComponentBase;
        AZ_COMPONENT(MeshBlockerComponent, MeshBlockerComponentTypeId, AreaComponentBase);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void Reflect(AZ::ReflectContext* context);

        MeshBlockerComponent(const MeshBlockerConfig& configuration);
        MeshBlockerComponent() = default;
        ~MeshBlockerComponent() = default;

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Activate() override;
        void Deactivate() override;
        bool ReadInConfig(const AZ::ComponentConfig* baseConfig) override;
        bool WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const override;

        //////////////////////////////////////////////////////////////////////////
        // AreaRequestBus
        bool PrepareToClaim(EntityIdStack& stackIds) override;
        void ClaimPositions(EntityIdStack& stackIds, ClaimContext& context) override;
        void UnclaimPosition(const ClaimHandle handle) override;

        //////////////////////////////////////////////////////////////////////////
        // AreaInfoBus
        AZ::Aabb GetEncompassingAabb() const override;
        AZ::u32 GetProductCount() const override;

        //////////////////////////////////////////////////////////////////////////
        // DependencyNotificationBus
        void OnCompositionChanged() override;

        //////////////////////////////////////////////////////////////////////////
        // MeshComponentNotificationBus
        void OnMeshCreated(const AZ::Data::Asset<AZ::Data::AssetData>& asset) override;
        void OnMeshDestroyed() override;
        void OnBoundsReset() override;

        //////////////////////////////////////////////////////////////////////////
        // SurfaceData::SurfaceDataSystemNotificationBus
        void OnSurfaceChanged(const AZ::EntityId& entityId, const AZ::Aabb& oldBounds, const AZ::Aabb& newBounds) override;

        ////////////////////////////////////////////////////////////////////////
        // AZ::TickBus
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;
    protected:
        ////////////////////////////////////////////////////////////////////////
        // MeshBlockerRequestBus
        AZ::u32 GetAreaPriority() const override;
        void SetAreaPriority(AZ::u32 priority) override;
        AZ::u32 GetAreaLayer() const override;
        void SetAreaLayer(AZ::u32 type) override;
        AZ::u32 GetAreaProductCount() const override;
        bool GetInheritBehavior() const override;
        void SetInheritBehavior(bool value) override;
        float GetMeshHeightPercentMin() const override;
        void SetMeshHeightPercentMin(float meshHeightPercentMin) override;
        float GetMeshHeightPercentMax() const override;
        void SetMeshHeightPercentMax(float meshHeightPercentMax) override;
        bool GetBlockWhenInvisible() const override;
        void SetBlockWhenInvisible(bool value) override;

    private:
        bool ClaimPosition(EntityIdStack& processedIds, const ClaimPoint& point, InstanceData& instanceData);
        void UpdateMeshData();

        MeshBlockerConfig m_configuration;
        AZStd::atomic_bool m_refresh{ false };

        // cached data
        mutable AZStd::recursive_mutex m_cacheMutex;
        AZ::Data::Asset<AZ::Data::AssetData> m_meshAssetData;
        AZ::Transform m_meshWorldTM = AZ::Transform::CreateIdentity();
        AZ::Transform m_meshWorldTMInverse = AZ::Transform::CreateIdentity();
        AZ::Aabb m_meshBounds = AZ::Aabb::CreateNull();
        AZ::Aabb m_meshBoundsForIntersection = AZ::Aabb::CreateNull();
        bool m_meshVisible = false;

        using CachedRayHits = AZStd::unordered_map<ClaimHandle, bool>;
        CachedRayHits m_cachedRayHits;
    };
}
