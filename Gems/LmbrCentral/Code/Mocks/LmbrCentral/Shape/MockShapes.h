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
}

