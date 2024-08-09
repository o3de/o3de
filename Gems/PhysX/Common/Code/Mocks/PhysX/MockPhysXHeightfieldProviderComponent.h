/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <gmock/gmock.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzFramework/Physics/HeightfieldProviderBus.h>

namespace UnitTest
{
    class MockPhysXHeightfieldProviderComponent
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(MockPhysXHeightfieldProviderComponent, "{C5F7CCCF-FDB2-40DF-992D-CF028F4A1B59}");

        static void Reflect([[maybe_unused]] AZ::ReflectContext* context)
        {
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<MockPhysXHeightfieldProviderComponent, AZ::Component>()
                    ->Version(1);
            }
        }

        void Activate() override
        {
        }

        void Deactivate() override
        {
        }

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC_CE("PhysicsHeightfieldProviderService"));
        }

    };

    class MockPhysXHeightfieldProvider
        : protected Physics::HeightfieldProviderRequestsBus::Handler
    {
    public:
        MockPhysXHeightfieldProvider(AZ::EntityId entityId)
        {
            Physics::HeightfieldProviderRequestsBus::Handler::BusConnect(entityId);
        }

        ~MockPhysXHeightfieldProvider()
        {
            Physics::HeightfieldProviderRequestsBus::Handler::BusDisconnect();
        }

        MOCK_CONST_METHOD0(GetHeightsAndMaterials, AZStd::vector<Physics::HeightMaterialPoint>());
        MOCK_CONST_METHOD0(GetHeightfieldGridSpacing, AZ::Vector2());
        MOCK_CONST_METHOD2(GetHeightfieldGridSize, void(size_t&, size_t&));
        MOCK_CONST_METHOD2(GetHeightfieldHeightBounds, void(float&, float&));
        MOCK_CONST_METHOD0(GetHeightfieldTransform, AZ::Transform());
        MOCK_CONST_METHOD0(GetMaterialList, AZStd::vector<AZ::Data::Asset<Physics::MaterialAsset>>());
        MOCK_CONST_METHOD0(GetHeights, AZStd::vector<float>());
        MOCK_CONST_METHOD1(UpdateHeights, AZStd::vector<float>(const AZ::Aabb& dirtyRegion));
        MOCK_CONST_METHOD0(GetHeightfieldAabb, AZ::Aabb());
        MOCK_CONST_METHOD0(GetHeightfieldMinHeight, float());
        MOCK_CONST_METHOD0(GetHeightfieldMaxHeight, float());
        MOCK_CONST_METHOD0(GetHeightfieldGridColumns, AZ::u64());
        MOCK_CONST_METHOD0(GetHeightfieldGridRows, AZ::u64());
        MOCK_CONST_METHOD5(GetHeightfieldIndicesFromRegion, void(const AZ::Aabb&, size_t&, size_t&, size_t&, size_t&));
        MOCK_CONST_METHOD5(
            UpdateHeightsAndMaterials, void(const Physics::UpdateHeightfieldSampleFunction&, size_t, size_t, size_t, size_t));
        MOCK_CONST_METHOD6(
            UpdateHeightsAndMaterialsAsync, void(const Physics::UpdateHeightfieldSampleFunction&,
                const Physics::UpdateHeightfieldCompleteFunction&, size_t, size_t, size_t, size_t));
    };
} // namespace UnitTest
