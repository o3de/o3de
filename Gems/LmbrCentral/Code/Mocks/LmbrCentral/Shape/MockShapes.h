/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <gmock/gmock.h>

#include <AzCore/Component/ComponentApplication.h>
#include <LmbrCentral/Shape/BoxShapeComponentBus.h>
#include <LmbrCentral/Shape/ShapeComponentBus.h>

namespace UnitTest
{
    class MockBoxShapeComponentRequests
        : public LmbrCentral::BoxShapeComponentRequestsBus::Handler
    {
    public:
        MockBoxShapeComponentRequests(AZ::EntityId entityId)
        {
            LmbrCentral::BoxShapeComponentRequestsBus::Handler::BusConnect(entityId);
        }

        ~MockBoxShapeComponentRequests()
        {
            LmbrCentral::BoxShapeComponentRequestsBus::Handler::BusDisconnect();
        }

        MOCK_METHOD0(GetBoxConfiguration, LmbrCentral::BoxShapeConfig());
        MOCK_METHOD0(GetBoxDimensions, AZ::Vector3());
        MOCK_METHOD1(SetBoxDimensions, void(const AZ::Vector3& newDimensions));
    };

    class MockShapeComponentRequests
        : public LmbrCentral::ShapeComponentRequestsBus::Handler
    {
    public:
        MockShapeComponentRequests(AZ::EntityId entityId)
        {
            LmbrCentral::ShapeComponentRequestsBus::Handler::BusConnect(entityId);
        }

        ~MockShapeComponentRequests()
        {
            LmbrCentral::ShapeComponentRequestsBus::Handler::BusDisconnect();
        }

        MOCK_METHOD0(GetShapeType, AZ::Crc32());
        MOCK_METHOD0(GetEncompassingAabb, AZ::Aabb());
        MOCK_METHOD2(GetTransformAndLocalBounds, void(AZ::Transform& transform, AZ::Aabb& bounds));
        MOCK_METHOD1(IsPointInside, bool(const AZ::Vector3& point));
        MOCK_METHOD1(DistanceSquaredFromPoint, float(const AZ::Vector3& point));
        MOCK_METHOD1(GenerateRandomPointInside, AZ::Vector3(AZ::RandomDistributionType randomDistribution));
        MOCK_METHOD3(IntersectRay, bool(const AZ::Vector3& src, const AZ::Vector3& dir, float& distance));
    };

    class MockShape : public LmbrCentral::ShapeComponentRequestsBus::Handler
    {
    public:
        AZ::Entity m_entity;
        int m_count = 0;

        MockShape()
        {
            LmbrCentral::ShapeComponentRequestsBus::Handler::BusConnect(m_entity.GetId());
        }

        ~MockShape()
        {
            LmbrCentral::ShapeComponentRequestsBus::Handler::BusDisconnect();
        }

        AZ::Crc32 GetShapeType() override
        {
            ++m_count;
            return AZ_CRC("TestShape", 0x856ca50c);
        }

        AZ::Aabb m_aabb = AZ::Aabb::CreateNull();
        AZ::Aabb GetEncompassingAabb() override
        {
            ++m_count;
            return m_aabb;
        }

        AZ::Transform m_localTransform = AZ::Transform::CreateIdentity();
        AZ::Aabb m_localBounds = AZ::Aabb::CreateNull();
        void GetTransformAndLocalBounds(AZ::Transform& transform, AZ::Aabb& bounds) override
        {
            ++m_count;
            transform = m_localTransform;
            bounds = m_localBounds;
        }

        bool m_pointInside = true;
        bool IsPointInside([[maybe_unused]] const AZ::Vector3& point) override
        {
            ++m_count;
            return m_pointInside;
        }

        float m_distanceSquaredFromPoint = 0.0f;
        float DistanceSquaredFromPoint([[maybe_unused]] const AZ::Vector3& point) override
        {
            ++m_count;
            return m_distanceSquaredFromPoint;
        }

        AZ::Vector3 m_randomPointInside = AZ::Vector3::CreateZero();
        AZ::Vector3 GenerateRandomPointInside([[maybe_unused]] AZ::RandomDistributionType randomDistribution) override
        {
            ++m_count;
            return m_randomPointInside;
        }

        bool m_intersectRay = false;
        bool IntersectRay(
            [[maybe_unused]] const AZ::Vector3& src, [[maybe_unused]] const AZ::Vector3& dir, [[maybe_unused]] float& distance) override
        {
            ++m_count;
            return m_intersectRay;
        }
    };
}

