/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/ComponentApplication.h>
#include <LmbrCentral/Shape/ShapeComponentBus.h>
#include <AzFramework/Physics/HeightfieldProviderBus.h>
#include <AzFramework/Terrain/TerrainDataRequestBus.h>
#include <TerrainSystem/TerrainSystemBus.h>

namespace UnitTest
{
    static const AZ::Uuid BoxShapeComponentTypeId = "{5EDF4B9E-0D3D-40B8-8C91-5142BCFC30A6}";

    class MockBoxShapeComponent
        : public AZ::Component
        , private LmbrCentral::ShapeComponentRequestsBus::Handler
    {
    public:
        AZ_COMPONENT(MockBoxShapeComponent, BoxShapeComponentTypeId)
        static void Reflect([[maybe_unused]] AZ::ReflectContext* context)
        {
        }

        void Activate() override
        {
            m_bounds = AZ::Aabb::CreateFromMinMax(AZ::Vector3(-100.0f, -100.0f, -100.0f), AZ::Vector3(100.0f, 100.0f, 100.0f));
            LmbrCentral::ShapeComponentRequestsBus::Handler::BusConnect(GetEntityId());
        }

        void Deactivate() override
        {
            LmbrCentral::ShapeComponentRequestsBus::Handler::BusDisconnect();
        }

        bool ReadInConfig([[maybe_unused]] const AZ::ComponentConfig* baseConfig) override
        {
            return true;
        }

        bool WriteOutConfig([[maybe_unused]] AZ::ComponentConfig* outBaseConfig) const override
        {
            return true;
        }

        void SetAabbFromMinMax(const AZ::Vector3& min, const AZ::Vector3& max)
        {
            m_bounds = AZ::Aabb::CreateFromMinMax(min, max);
        }

    private:
        static void GetProvidedServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("ShapeService", 0xe86aa5fe));
            provided.push_back(AZ_CRC("BoxShapeService", 0x946a0032));
            provided.push_back(AZ_CRC_CE("AxisAlignedBoxShapeService"));
        }

        static void GetIncompatibleServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
        }

        static void GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
        {
        }

        static void GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
        {
        }

        //ShapeComponentRequestsBus
        AZ::Crc32 GetShapeType() override
        {
            return AZ_CRC("Box", 0x08a9483a);
        }

        AZ::Aabb GetEncompassingAabb() override
        {
            return m_bounds;
        }

        virtual void GetTransformAndLocalBounds(AZ::Transform& transform, AZ::Aabb& bounds) override
        {
            transform = AZ::Transform::Identity();
            bounds.CreateFromMinMax(AZ::Vector3(-1.0f, -1.0f, -1.0f), AZ::Vector3(1.0f, 1.0f, 1.0f));
        }

        virtual bool IsPointInside([[maybe_unused]] const AZ::Vector3& point)override
        {
            return true;
        }

        virtual float DistanceSquaredFromPoint([[maybe_unused]] const AZ::Vector3& point) override
        {
            return 1.0f;
        }

        AZ::Aabb m_bounds;
    };

    class MockTerrainSystem
        : private Terrain::TerrainSystemServiceRequestBus::Handler
        , private AzFramework::Terrain::TerrainDataRequestBus::Handler
    {
    public:

        void Activate() override
        {
            Terrain::TerrainSystemServiceRequestBus::Handler::BusConnect();
            AzFramework::Terrain::TerrainDataRequestBus::Handler::BusConnect();
        }

        void Deactivate() override
        {
            AzFramework::Terrain::TerrainDataRequestBus::Handler::BusDisconnect();
            Terrain::TerrainSystemServiceRequestBus::Handler::BusDisconnect();
        }

        void SetMockHeight(const float height)
        {
            m_height = height;
        }

        void SetWorldBounds([[maybe_unused]] const AZ::Aabb& worldBounds) override
        {
        }

        void SetHeightQueryResolution([[maybe_unused]] AZ::Vector2 queryResolution) override
        {
        }

        void RegisterArea([[maybe_unused]] AZ::EntityId areaId) override
        {
            m_registerAreaCalledCount++;
        }

        void UnregisterArea([[maybe_unused]] AZ::EntityId areaId) override
        {
            m_unregisterAreaCalledCount++;
        }

        void RefreshArea([[maybe_unused]] AZ::EntityId areaId) override
        {
            m_refreshAreaCalledCount++;
        }

        // TerrainDataRequestBus
        AZ::Vector2 GetTerrainGridResolution() const override
        {
            return AZ::Vector2(1.0f);
        }

        AZ::Aabb GetTerrainAabb() const override
        {
            return {};
        }

        float GetHeight(AZ::Vector3 /*position*/, Sampler /*sampler*/, bool* /* terrainExistsPtr*/) const override
        {
            return m_height;
        }

        float GetHeightFromFloats(float /*x*/, float /*y*/, Sampler /*sampler*/, bool* /*terrainExistsPtr*/) const override
        {
            return m_height;
        }

        AzFramework::SurfaceData::SurfaceTagWeight GetMaxSurfaceWeight(
            AZ::Vector3 /*position*/, Sampler /*sampleFilter*/, bool* /*terrainExistsPtr*/) const override
        {
            AzFramework::SurfaceData::SurfaceTagWeight weight;
            weight.m_weight = 1.0f;
            return weight;
        }

        AzFramework::SurfaceData::SurfaceTagWeight GetMaxSurfaceWeightFromFloats(float /*x*/, float /*y*/, Sampler /*sampleFilter*/, bool* /*terrainExistsPtr*/) const override
        {
            AzFramework::SurfaceData::SurfaceTagWeight weight;
            weight.m_weight = 1.0f;
            return weight;
        }

        const char* GetMaxSurfaceName(AZ::Vector3 /*position*/, Sampler /*sampleFilter*/, bool* /*terrainExistsPtr*/) const override
        {
            return {};
        }

        bool GetIsHoleFromFloats(float /*x*/, float /*y*/, Sampler /*sampleFilter*/) const override
        {
            return {};
        }

        AZ::Vector3 GetNormal(AZ::Vector3 /*position*/, Sampler /*sampleFilter*/, bool* /*terrainExistsPtr*/) const override
        {
            return {};
        }

        AZ::Vector3 GetNormalFromFloats(float /*x*/, float /*y*/, Sampler /*sampleFilter*/, bool* /*terrainExistsPtr*/) const override
        {
            return {};
        }

        int m_registerAreaCalledCount = 0;
        int m_refreshAreaCalledCount = 0;
        int m_unregisterAreaCalledCount = 0;

        float m_height = 1.0f;
    };

    class MockHeightfieldProviderNotificationBusListener
        : public AZ::Component
        , private Physics::HeightfieldProviderNotificationBus::Handler
    {
    public:
        AZ_COMPONENT(MockHeightfieldProviderNotificationBusListener, "{277D39B9-F485-4259-84A4-78E97C687614}");

        static void Reflect([[maybe_unused]] AZ::ReflectContext* context)
        {
        }

        void Activate()
        {
            Physics::HeightfieldProviderNotificationBus::Handler::BusConnect(GetEntityId());
        }

        void Deactivate()
        {
            Physics::HeightfieldProviderNotificationBus::Handler::BusDisconnect();
        }

        void OnHeightfieldDataChanged([[maybe_unused]] const AZ::Aabb& dirtyRegion) override
        {
            m_onHeightfieldDataChangedCalledCount++;
        }

        int m_onHeightfieldDataChangedCalledCount = 0;
        int m_refreshHeightfieldCalledCount = 0;
    };
    
}
