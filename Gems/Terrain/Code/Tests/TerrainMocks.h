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
#include <AzFramework/Physics/HeightfieldProviderComponentBus.h>

namespace UnitTest
{
    static const AZ::Uuid BoxShapeComponentTypeId = "{5EDF4B9E-0D3D-40B8-8C91-5142BCFC30A6}";

    class MockBoxShapeComponent
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(MockBoxShapeComponent, BoxShapeComponentTypeId)
        static void Reflect([[maybe_unused]] AZ::ReflectContext* context)
        {
        }

        void Activate() override
        {
        }

        void Deactivate() override
        {
        }

        bool ReadInConfig([[maybe_unused]] const AZ::ComponentConfig* baseConfig) override
        {
            return true;
        }

        bool WriteOutConfig([[maybe_unused]] AZ::ComponentConfig* outBaseConfig) const override
        {
            return true;
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
    };

    class MockTerrainSystem : private Terrain::TerrainSystemServiceRequestBus::Handler
    {
    public:
        void Activate() override
        {
            Terrain::TerrainSystemServiceRequestBus::Handler::BusConnect();
        }

        void Deactivate() override
        {
            Terrain::TerrainSystemServiceRequestBus::Handler::BusDisconnect();
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

        int m_registerAreaCalledCount = 0;
        int m_refreshAreaCalledCount = 0;
        int m_unregisterAreaCalledCount = 0;
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

        void RefreshHeightfield() override
        {
            m_refreshHeightfieldCalledCount++;
        }

        int m_onHeightfieldDataChangedCalledCount = 0;
        int m_refreshHeightfieldCalledCount = 0;
    };
    
}
