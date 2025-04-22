/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "VegetationTest.h"
#include "VegetationMocks.h"

#include <AzTest/AzTest.h>
#include <AzCore/UnitTest/TestTypes.h>

#include <Source/Components/PositionModifierComponent.h>
#include <Source/Components/RotationModifierComponent.h>
#include <Source/Components/ScaleModifierComponent.h>
#include <Source/Components/SlopeAlignmentModifierComponent.h>

namespace UnitTest
{
    struct VegetationComponentModifierTests
        : public VegetationComponentTests
    {
        Vegetation::InstanceData m_instanceData;

        void RegisterComponentDescriptors() override
        {
            m_app.RegisterComponentDescriptor(MockShapeServiceComponent::CreateDescriptor());
            m_app.RegisterComponentDescriptor(MockVegetationAreaServiceComponent::CreateDescriptor());
            m_app.RegisterComponentDescriptor(MockMeshServiceComponent::CreateDescriptor());
        }
    };

    TEST_F(VegetationComponentModifierTests, PositionModifierComponent)
    {
        const auto crcMask = AZ_CRC_CE("mock-mask");

        Vegetation::InstanceData vegInstance;
        vegInstance.m_position = AZ::Vector3(2.0f, 4.0f, 0.0f);

        MockGradientRequestHandler gradient;
        gradient.m_defaultValue = 0.99f;

        Vegetation::PositionModifierConfig config;
        config.m_autoSnapToSurface = false;
        config.m_rangeMinX = -0.3f;
        config.m_rangeMaxX = 0.3f;
        config.m_gradientSamplerX.m_gradientId = gradient.m_entity.GetId();

        config.m_rangeMinY = -0.3f;
        config.m_rangeMaxY = 0.3f;
        config.m_gradientSamplerY.m_gradientId = gradient.m_entity.GetId();

        config.m_rangeMinZ = 0.0f;
        config.m_rangeMaxZ = 0.0f;
        config.m_gradientSamplerZ.m_gradientId = gradient.m_entity.GetId();

        Vegetation::PositionModifierComponent* component = nullptr;
        auto entity = CreateEntity(config, &component, [](AZ::Entity* e)
        {
            e->CreateComponent<MockVegetationAreaServiceComponent>();
        });

        Vegetation::ModifierRequestBus::Event(entity->GetId(), &Vegetation::ModifierRequestBus::Events::Execute, vegInstance);

        EXPECT_NEAR(vegInstance.m_position.GetX(), 2.294f, AZ::Constants::Tolerance);
        EXPECT_NEAR(vegInstance.m_position.GetY(), 4.294f, AZ::Constants::Tolerance);
        EXPECT_NEAR(vegInstance.m_position.GetZ(), 0.0f, AZ::Constants::Tolerance);

        // with the surface handler
        MockSurfaceHandler mockSurfaceHandler;
        mockSurfaceHandler.m_outPosition = AZ::Vector3(vegInstance.m_position.GetX(), vegInstance.m_position.GetY(), 6.0f);
        mockSurfaceHandler.m_outNormal = AZ::Vector3(0.0f, 0.0f, 1.0f);
        mockSurfaceHandler.m_outMasks.AddSurfaceTagWeight(crcMask, 1.0f);

        entity->Deactivate();
        config.m_autoSnapToSurface = true;
        component->ReadInConfig(&config);
        entity->Activate();

        Vegetation::ModifierRequestBus::Event(entity->GetId(), &Vegetation::ModifierRequestBus::Events::Execute, vegInstance);
        EXPECT_EQ(mockSurfaceHandler.m_outNormal, vegInstance.m_normal);
        EXPECT_EQ(mockSurfaceHandler.m_outMasks, vegInstance.m_masks);
    }

    TEST_F(VegetationComponentModifierTests, RotationModifierComponent)
    {
        m_instanceData.m_rotation = AZ::Quaternion::CreateIdentity();

        MockGradientRequestHandler gradientX;
        gradientX.m_defaultValue = 0.15f;

        MockGradientRequestHandler gradientY;
        gradientY.m_defaultValue = 0.25f;

        MockGradientRequestHandler gradientZ;
        gradientZ.m_defaultValue = 0.35f;

        Vegetation::RotationModifierConfig config;
        config.m_rangeMinX = -100.0f;
        config.m_rangeMaxX = 100.0f;
        config.m_gradientSamplerX.m_gradientId = gradientX.m_entity.GetId();

        config.m_rangeMinY = -80.0f;
        config.m_rangeMaxY = 80.0f;
        config.m_gradientSamplerY.m_gradientId = gradientY.m_entity.GetId();

        config.m_rangeMinZ = -180.0f;
        config.m_rangeMaxZ = 180.0f;
        config.m_gradientSamplerZ.m_gradientId = gradientZ.m_entity.GetId();

        Vegetation::RotationModifierComponent* component = nullptr;
        auto entity = CreateEntity(config, &component, [](AZ::Entity* e)
        {
            e->CreateComponent<MockVegetationAreaServiceComponent>();
        });

        Vegetation::ModifierRequestBus::Event(entity->GetId(), &Vegetation::ModifierRequestBus::Events::Execute, m_instanceData);

        EXPECT_NEAR(m_instanceData.m_rotation.GetW(), 0.777f, 0.003f);
        EXPECT_NEAR(m_instanceData.m_rotation.GetX(), -0.353f, 0.003f);
        EXPECT_NEAR(m_instanceData.m_rotation.GetY(), -0.495f, 0.003f);
        EXPECT_NEAR(m_instanceData.m_rotation.GetZ(), -0.175f, 0.003f);
    }

    TEST_F(VegetationComponentModifierTests, ScaleModifierComponent)
    {
        MockGradientRequestHandler gradient;
        gradient.m_defaultValue = 0.1234f;

        Vegetation::ScaleModifierConfig config;
        config.m_gradientSampler.m_gradientId = gradient.m_entity.GetId();
        config.m_rangeMin = 0.1f;
        config.m_rangeMax = 0.9f;

        m_instanceData.m_scale = 1.0f;

        Vegetation::ScaleModifierComponent* component = nullptr;
        auto entity = CreateEntity(config, &component, [](AZ::Entity* e)
        {
            e->CreateComponent<MockVegetationAreaServiceComponent>();
        });

        Vegetation::ModifierRequestBus::Event(entity->GetId(), &Vegetation::ModifierRequestBus::Events::Execute, m_instanceData);

        EXPECT_TRUE(AZ::IsClose(m_instanceData.m_scale, 0.19872f, std::numeric_limits<decltype(m_instanceData.m_scale)>::epsilon()));
    }

#if AZ_TRAIT_DISABLE_FAILED_VEGETATION_TESTS
    TEST_F(VegetationComponentModifierTests, DISABLED_SlopeAlignmentModifierComponent)
#else
    TEST_F(VegetationComponentModifierTests, SlopeAlignmentModifierComponent)
#endif // AZ_TRAIT_DISABLE_FAILED_VEGETATION_TESTS
    {
        MockGradientRequestHandler gradient;
        gradient.m_defaultValue = 0.87654f;

        Vegetation::SlopeAlignmentModifierConfig config;
        config.m_gradientSampler.m_gradientId = gradient.m_entity.GetId();
        config.m_rangeMin = 0.1f;
        config.m_rangeMax = 0.9f;

        m_instanceData.m_normal = AZ::Vector3(0.12f, 0.34f, 0.56f);
        m_instanceData.m_alignment = AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3::CreateAxisY(), AZ::DegToRad(42)).GetNormalized();

        Vegetation::SlopeAlignmentModifierComponent* component = nullptr;
        auto entity = CreateEntity(config, &component, [](AZ::Entity* e)
        {
            e->CreateComponent<MockVegetationAreaServiceComponent>();
        });

        Vegetation::ModifierRequestBus::Event(entity->GetId(), &Vegetation::ModifierRequestBus::Events::Execute, m_instanceData);

        EXPECT_NEAR(m_instanceData.m_alignment.GetX(), -0.1973f, AZ::Constants::Tolerance);
        EXPECT_NEAR(m_instanceData.m_alignment.GetY(),  0.0666f, AZ::Constants::Tolerance);
        EXPECT_NEAR(m_instanceData.m_alignment.GetZ(), -0.0134f, AZ::Constants::Tolerance);
        EXPECT_NEAR(m_instanceData.m_alignment.GetW(),  0.9779f, AZ::Constants::Tolerance);
    }
}

