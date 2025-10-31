/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Vegetation/Ebuses/AreaNotificationBus.h>
#include <Vegetation/Ebuses/AreaRequestBus.h>
#include <Vegetation/Ebuses/AreaSystemRequestBus.h>
#include <Vegetation/Ebuses/DescriptorProviderRequestBus.h>
#include <Vegetation/Ebuses/DescriptorSelectorRequestBus.h>
#include <Vegetation/Ebuses/FilterRequestBus.h>
#include <Vegetation/Ebuses/InstanceSystemRequestBus.h>
#include <Vegetation/Ebuses/ModifierRequestBus.h>
#include <Vegetation/Ebuses/SystemConfigurationBus.h>
#include <LmbrCentral/Shape/ShapeComponentBus.h>
#include <AtomLyIntegration/CommonFeatures/Mesh/MeshComponentBus.h>
#include <AzCore/Math/Random.h>
#include <GradientSignal/Ebuses/GradientRequestBus.h>
#include <SurfaceData/SurfaceDataSystemRequestBus.h>
#include <SurfaceData/Utility/SurfaceDataUtility.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/std/containers/set.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <Atom/RPI.Reflect/Model/ModelAsset.h>
#include <Source/AreaSystemComponent.h>
#include <Source/InstanceSystemComponent.h>
#include <ISerialize.h>
#include <IIndexedMesh.h>

//////////////////////////////////////////////////////////////////////////
// mock event bus classes for testing vegetation
namespace UnitTest
{
    struct MockAreaManager
        : public Vegetation::AreaSystemRequestBus::Handler
    {
        mutable int m_count = 0;

        MockAreaManager()
        {
            Vegetation::AreaSystemRequestBus::Handler::BusConnect();
        }

        ~MockAreaManager()
        {
            Vegetation::AreaSystemRequestBus::Handler::BusDisconnect();
        }

        void RegisterArea([[maybe_unused]] AZ::EntityId areaId, [[maybe_unused]] AZ::u32 layer, [[maybe_unused]] AZ::u32 priority, [[maybe_unused]] const AZ::Aabb& bounds) override
        {
            ++m_count;
        }

        void UnregisterArea([[maybe_unused]] AZ::EntityId areaId) override
        {
            ++m_count;
        }

        void RefreshArea([[maybe_unused]] AZ::EntityId areaId, [[maybe_unused]] AZ::u32 layer, [[maybe_unused]] AZ::u32 priority, [[maybe_unused]] const AZ::Aabb& bounds) override
        {
            ++m_count;
        }

        void RefreshAllAreas() override
        {
            ++m_count;
        }

        void ClearAllAreas() override
        {
            ++m_count;
        }

        void MuteArea([[maybe_unused]] AZ::EntityId areaId) override
        {
            ++m_count;
        }

        void UnmuteArea([[maybe_unused]] AZ::EntityId areaId) override
        {
            ++m_count;
        }

        AZStd::vector<Vegetation::InstanceData> m_existingInstances;
        void EnumerateInstancesInOverlappingSectors(const AZ::Aabb& bounds, Vegetation::AreaSystemEnumerateCallback callback) const override
        {
            EnumerateInstancesInAabb(bounds, callback);
        }

        void EnumerateInstancesInAabb([[maybe_unused]] const AZ::Aabb& bounds, Vegetation::AreaSystemEnumerateCallback callback) const override
        {
            ++m_count;
            for (const auto& instanceData : m_existingInstances)
            {
                if (callback(instanceData) != Vegetation::AreaSystemEnumerateCallbackResult::KeepEnumerating)
                {
                    return;
                }
            }
        }

        AZStd::size_t GetInstanceCountInAabb([[maybe_unused]] const AZ::Aabb& bounds) const override
        {
            ++m_count;
            return m_existingInstances.size();
        }

        AZStd::vector<Vegetation::InstanceData> GetInstancesInAabb([[maybe_unused]] const AZ::Aabb& bounds) const override
        {
            ++m_count;
            return m_existingInstances;
        }
    };

    struct MockDescriptorBus
        : public Vegetation::InstanceSystemRequestBus::Handler
    {
        AZStd::set<Vegetation::DescriptorPtr> m_descriptorSet;

        MockDescriptorBus()
        {
            Vegetation::InstanceSystemRequestBus::Handler::BusConnect();
        }

        ~MockDescriptorBus() override
        {
            Vegetation::InstanceSystemRequestBus::Handler::BusDisconnect();
        }

        Vegetation::DescriptorPtr RegisterUniqueDescriptor(const Vegetation::Descriptor& descriptor) override
        {
            Vegetation::DescriptorPtr sharedPtr = AZStd::make_shared<Vegetation::Descriptor>(descriptor);
            m_descriptorSet.insert(sharedPtr);
            return sharedPtr;
        }

        void ReleaseUniqueDescriptor(Vegetation::DescriptorPtr descriptorPtr) override
        {
            m_descriptorSet.erase(descriptorPtr);
        }

        void CreateInstance(Vegetation::InstanceData& instanceData) override
        {
            instanceData.m_instanceId = Vegetation::InstanceId();
        }

        void DestroyInstance([[maybe_unused]] Vegetation::InstanceId instanceId) override {}

        void DestroyAllInstances() override {}

        void Cleanup() override {}
    };

    struct MockGradientRequestHandler
        : public GradientSignal::GradientRequestBus::Handler
    {
        mutable int m_count = 0;
        AZStd::function<float()> m_valueGetter;
        float m_defaultValue = -AZ::Constants::FloatMax;
        AZ::Entity m_entity;

        MockGradientRequestHandler()
        {
            GradientSignal::GradientRequestBus::Handler::BusConnect(m_entity.GetId());
        }

        ~MockGradientRequestHandler()
        {
            GradientSignal::GradientRequestBus::Handler::BusDisconnect();
        }

        float GetValue([[maybe_unused]] const GradientSignal::GradientSampleParams& sampleParams) const override
        {
            ++m_count;

            if (m_valueGetter)
            {
                return m_valueGetter();
            }
            return m_defaultValue;
        }

        bool IsEntityInHierarchy(const AZ::EntityId &) const override
        {
            return false;
        }
    };

    struct MockShapeComponentNotificationsBus
        : public LmbrCentral::ShapeComponentRequestsBus::Handler
    {
        AZ::Aabb m_aabb = AZ::Aabb::CreateCenterRadius(AZ::Vector3::CreateZero(), AZ::Constants::FloatMax);

        MockShapeComponentNotificationsBus()
        {
        }

        ~MockShapeComponentNotificationsBus() override
        {
        }

        void GetTransformAndLocalBounds(AZ::Transform& transform, AZ::Aabb& bounds) const override
        {
            transform = AZ::Transform::CreateTranslation(m_aabb.GetCenter());
            bounds = m_aabb;
        }

        AZ::Crc32 GetShapeType() const override
        {
            return AZ::Crc32();
        }

        AZ::Aabb GetEncompassingAabb() const override
        {
            return m_aabb;
        }

        bool IsPointInside(const AZ::Vector3& point) const override
        {
            return m_aabb.Contains(point);
        }

        float DistanceSquaredFromPoint(const AZ::Vector3& point) const override
        {
            return m_aabb.GetDistanceSq(point);
        }
    };

    struct MockSystemConfigurationRequestBus
        : public Vegetation::SystemConfigurationRequestBus::Handler
    {
        AZ::ComponentConfig* m_lastUpdated = nullptr;
        mutable const AZ::ComponentConfig* m_lastRead = nullptr;
        Vegetation::AreaSystemConfig m_areaSystemConfig;
        Vegetation::InstanceSystemConfig m_instanceSystemConfig;

        MockSystemConfigurationRequestBus()
        {
            Vegetation::SystemConfigurationRequestBus::Handler::BusConnect();
        }

        ~MockSystemConfigurationRequestBus() override
        {
            Vegetation::SystemConfigurationRequestBus::Handler::BusDisconnect();
        }

        void UpdateSystemConfig(const AZ::ComponentConfig* config) override
        {
            if (azrtti_typeid(m_areaSystemConfig) == azrtti_typeid(*config))
            {
                m_areaSystemConfig = *azrtti_cast<const Vegetation::AreaSystemConfig*>(config);
                m_lastUpdated = &m_areaSystemConfig;
            }
            else if (azrtti_typeid(m_instanceSystemConfig) == azrtti_typeid(*config))
            {
                m_instanceSystemConfig = *azrtti_cast<const Vegetation::InstanceSystemConfig*>(config);
                m_lastUpdated = &m_instanceSystemConfig;
            }
            else
            {
                m_lastUpdated = nullptr;
                AZ_Error("vegetation", false, "Invalid AZ::ComponentConfig type updated");
            }
        }

        void GetSystemConfig(AZ::ComponentConfig* config) const override
        {
            if (azrtti_typeid(m_areaSystemConfig) == azrtti_typeid(*config))
            {
                *azrtti_cast<Vegetation::AreaSystemConfig*>(config) = m_areaSystemConfig;
                m_lastRead = azrtti_cast<const Vegetation::AreaSystemConfig*>(&m_areaSystemConfig);
            }
            else if (azrtti_typeid(m_instanceSystemConfig) == azrtti_typeid(*config))
            {
                *azrtti_cast<Vegetation::InstanceSystemConfig*>(config) = m_instanceSystemConfig;
                m_lastRead = azrtti_cast<const Vegetation::InstanceSystemConfig*>(&m_instanceSystemConfig);
            }
            else
            {
                m_lastRead = nullptr;
                AZ_Error("vegetation", false, "Invalid AZ::ComponentConfig type read");
            }
        }
    };

    template<class T>
    struct MockAsset : public AZ::Data::Asset<T>
    {
        void ClearData()
        {
            this->m_assetData = nullptr;
        }
    };

    struct MockAssetData
        : public AZ::Data::AssetData
    {
        void SetId(const AZ::Data::AssetId& assetId)
        {
            m_assetId = assetId;
            m_status.store(AZ::Data::AssetData::AssetStatus::Ready);
        }

        void SetStatus(AZ::Data::AssetData::AssetStatus status)
        {
            m_status.store(status);
        }
    };

    struct MockSurfaceHandler
        : public SurfaceData::SurfaceDataSystemRequestBus::Handler
    {
    public:
        mutable int m_count = 0;

        MockSurfaceHandler()
        {
            AZ::Interface<SurfaceData::SurfaceDataSystem>::Register(this);
        }

        ~MockSurfaceHandler()
        {
            AZ::Interface<SurfaceData::SurfaceDataSystem>::Unregister(this);
        }

        AZ::Vector3 m_outPosition = {};
        AZ::Vector3 m_outNormal = {};
        SurfaceData::SurfaceTagWeights m_outMasks;
        void GetSurfacePoints([[maybe_unused]] const AZ::Vector3& inPosition, [[maybe_unused]] const SurfaceData::SurfaceTagVector& masks, SurfaceData::SurfacePointList& surfacePointList) const override
        {
            ++m_count;
            surfacePointList.Clear();
            surfacePointList.StartListConstruction(AZStd::span<const AZ::Vector3>(&inPosition, 1), 1, {});
            surfacePointList.AddSurfacePoint(AZ::EntityId(), inPosition, m_outPosition, m_outNormal, m_outMasks);
            surfacePointList.EndListConstruction();
        }

        void GetSurfacePointsFromRegion([[maybe_unused]] const AZ::Aabb& inRegion, [[maybe_unused]] const AZ::Vector2 stepSize, [[maybe_unused]] const SurfaceData::SurfaceTagVector& desiredTags,
            [[maybe_unused]] SurfaceData::SurfacePointList& surfacePointListPerPosition) const override
        {
        }

        void GetSurfacePointsFromList(
            [[maybe_unused]] AZStd::span<const AZ::Vector3> inPositions,
            [[maybe_unused]] const SurfaceData::SurfaceTagVector& desiredTags,
            [[maybe_unused]] SurfaceData::SurfacePointList& surfacePointLists) const override
        {
        }

        SurfaceData::SurfaceDataRegistryHandle RegisterSurfaceDataProvider([[maybe_unused]] const SurfaceData::SurfaceDataRegistryEntry& entry) override
        {
            ++m_count;
            return SurfaceData::InvalidSurfaceDataRegistryHandle;
        }

        void UnregisterSurfaceDataProvider([[maybe_unused]] const SurfaceData::SurfaceDataRegistryHandle& handle) override
        {
            ++m_count;
        }

        void UpdateSurfaceDataProvider([[maybe_unused]] const SurfaceData::SurfaceDataRegistryHandle& handle, [[maybe_unused]] const SurfaceData::SurfaceDataRegistryEntry& entry) override
        {
            ++m_count;
        }

        SurfaceData::SurfaceDataRegistryHandle RegisterSurfaceDataModifier([[maybe_unused]] const SurfaceData::SurfaceDataRegistryEntry& entry) override
        {
            ++m_count;
            return SurfaceData::InvalidSurfaceDataRegistryHandle;
        }

        void UnregisterSurfaceDataModifier([[maybe_unused]] const SurfaceData::SurfaceDataRegistryHandle& handle) override
        {
            ++m_count;
        }

        void UpdateSurfaceDataModifier([[maybe_unused]] const SurfaceData::SurfaceDataRegistryHandle& handle, [[maybe_unused]] const SurfaceData::SurfaceDataRegistryEntry& entry) override
        {
            ++m_count;
        }

        void RefreshSurfaceData(
            [[maybe_unused]] const SurfaceData::SurfaceDataRegistryHandle& handle, [[maybe_unused]] const AZ::Aabb& dirtyBounds) override
        {
            ++m_count;
        }

        SurfaceData::SurfaceDataRegistryHandle GetSurfaceDataProviderHandle([[maybe_unused]] const AZ::EntityId& providerEntityId) override
        {
            return {};
        }

        SurfaceData::SurfaceDataRegistryHandle GetSurfaceDataModifierHandle([[maybe_unused]] const AZ::EntityId& modifierEntityId) override
        {
            return {};
        }
    };

    struct MockMeshAsset
        : public AZ::RPI::ModelAsset
    {
        AZ_RTTI(MockMeshAsset, "{C314B960-9B54-468D-B37C-065738E7487C}", AZ::RPI::ModelAsset);
        AZ_CLASS_ALLOCATOR(ModelAsset, AZ::SystemAllocator);

        MockMeshAsset()
        {
            m_assetId = AZ::Uuid::CreateRandom();
            m_status.store(AZ::Data::AssetData::AssetStatus::Ready);
            m_useCount.fetch_add(1);
            m_weakUseCount.fetch_add(1);
        }

        ~MockMeshAsset() override = default;

        bool LocalRayIntersectionAgainstModel(
            [[maybe_unused]] const AZ::Vector3& rayStart, [[maybe_unused]] const AZ::Vector3& dir, [[maybe_unused]] bool allowBruteForce,
            [[maybe_unused]] float& distance, [[maybe_unused]] AZ::Vector3& normal) const override
        {
            distance = 0.1f;
            return true;
        }
    };

    struct MockMeshRequestBus
        : public AZ::Render::MeshComponentRequestBus::Handler
    {
        AZ::Aabb m_GetWorldBoundsOutput;
        AZ::Aabb GetWorldBounds() const override
        {
            return m_GetWorldBoundsOutput;
        }

        AZ::Aabb m_GetLocalBoundsOutput;
        AZ::Aabb GetLocalBounds() const override
        {
            return m_GetLocalBoundsOutput;
        }

        void SetModelAsset([[maybe_unused]] AZ::Data::Asset<AZ::RPI::ModelAsset> modelAsset) override
        {
        }

        AZ::Data::Asset<AZ::RPI::ModelAsset> m_GetMeshAssetOutput;
        AZ::Data::Asset<const AZ::RPI::ModelAsset> GetModelAsset() const override
        {
            return m_GetMeshAssetOutput;
        }

        bool m_GetVisibilityOutput = true;
        bool GetVisibility() const override
        {
            return m_GetVisibilityOutput;
        }
        void SetVisibility(bool visibility) override
        {
            m_GetVisibilityOutput = visibility;
        }

        void SetRayTracingEnabled([[maybe_unused]] bool enabled) override
        {
        }

        bool GetRayTracingEnabled() const override
        {
            return false;
        }

        void SetExcludeFromReflectionCubeMaps([[maybe_unused]] bool excludeFromReflectionCubeMaps) override
        {
        }

        bool GetExcludeFromReflectionCubeMaps() const override
        {
            return false;
        }

        AZ::Data::AssetId m_assetIdOutput;
        void SetModelAssetId(AZ::Data::AssetId modelAssetId) override
        {
            m_assetIdOutput = modelAssetId;
        }
        AZ::Data::AssetId GetModelAssetId() const override
        {
            return m_assetIdOutput;
        }

        AZStd::string m_modelAssetPathOutput;
        void SetModelAssetPath(const AZStd::string& path) override
        {
            m_modelAssetPathOutput = path;
        }
        AZStd::string GetModelAssetPath() const override
        {
            return m_modelAssetPathOutput;
        }

        AZ::Data::Instance<AZ::RPI::Model> GetModel() const override
        {
            return AZ::Data::Instance<AZ::RPI::Model>();
        }

        AZ::RHI::DrawItemSortKey m_drawItemSortKeyOutput;
        void SetSortKey(AZ::RHI::DrawItemSortKey sortKey) override
        {
            m_drawItemSortKeyOutput = sortKey;
        }
        AZ::RHI::DrawItemSortKey GetSortKey() const override
        {
            return m_drawItemSortKeyOutput;
        }

        bool m_isAlwaysDynamic = false;
        void SetIsAlwaysDynamic(bool isAlwaysDynamic) override
        {
            m_isAlwaysDynamic = isAlwaysDynamic;
        }
        bool GetIsAlwaysDynamic() const override
        {
            return m_isAlwaysDynamic;
        }

        AZ::RPI::Cullable::LodType m_lodTypeOutput;
        void SetLodType(AZ::RPI::Cullable::LodType lodType) override
        {
            m_lodTypeOutput = lodType;
        }
        AZ::RPI::Cullable::LodType GetLodType() const override
        {
            return m_lodTypeOutput;
        }

        AZ::RPI::Cullable::LodOverride m_lodOverrideOutput;
        void SetLodOverride(AZ::RPI::Cullable::LodOverride lodOverride) override
        {
            m_lodOverrideOutput = lodOverride;
        }
        AZ::RPI::Cullable::LodOverride GetLodOverride() const override
        {
            return m_lodOverrideOutput;
        }

        float m_minimumScreenCoverageOutput;
        void SetMinimumScreenCoverage(float minimumScreenCoverage) override
        {
            m_minimumScreenCoverageOutput = minimumScreenCoverage;
        }
        float GetMinimumScreenCoverage() const override
        {
            return m_minimumScreenCoverageOutput;
        }

        float m_qualityDecayRateOutput;
        void SetQualityDecayRate(float qualityDecayRate) override
        {
            m_qualityDecayRateOutput = qualityDecayRate;
        }
        float GetQualityDecayRate() const override
        {
            return m_qualityDecayRateOutput;
        }
    };

    struct MockTransformBus
        : public AZ::TransformBus::Handler
    {
        void BindTransformChangedEventHandler(AZ::TransformChangedEvent::Handler&) override
        {
            ;
        }
        void BindParentChangedEventHandler(AZ::ParentChangedEvent::Handler&) override
        {
            ;
        }
        void BindChildChangedEventHandler(AZ::ChildChangedEvent::Handler&) override
        {
            ;
        }
        void NotifyChildChangedEvent(AZ::ChildChangeType, AZ::EntityId) override
        {
            ;
        }
        AZ::Transform m_GetLocalTMOutput;
        const AZ::Transform & GetLocalTM() override
        {
            return m_GetLocalTMOutput;
        }
        AZ::Transform m_GetWorldTMOutput;
        const AZ::Transform & GetWorldTM() override
        {
            return m_GetWorldTMOutput;
        }
        bool IsStaticTransform() override
        {
            return false;
        }
    };

    class MockShapeServiceComponent
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(MockShapeServiceComponent, "{E1687D77-F43F-439B-BB6D-B1457E94812A}", AZ::Component);

        void Activate() override {}
        void Deactivate() override {}

        static void Reflect(AZ::ReflectContext* reflect) { AZ_UNUSED(reflect); }
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC_CE("ShapeService"));
            provided.push_back(AZ_CRC_CE("VegetationDescriptorProviderService"));
        }
    };

    class MockVegetationAreaServiceComponent
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(MockVegetationAreaServiceComponent, "{EF5292D8-411E-4660-9B31-EFAEED34B1EE}", AZ::Component);

        void Activate() override {}
        void Deactivate() override {}

        static void Reflect(AZ::ReflectContext* reflect) { AZ_UNUSED(reflect); }
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC_CE("VegetationAreaService"));
        }
    };

    class MockMeshServiceComponent
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(MockMeshServiceComponent, "{69547762-7EAB-4AC4-86FA-7486F1BBB115}", AZ::Component);

        void Activate() override {}
        void Deactivate() override {}

        static void Reflect(AZ::ReflectContext* reflect) { AZ_UNUSED(reflect); }
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC_CE("MeshService"));
        }
    };
}
