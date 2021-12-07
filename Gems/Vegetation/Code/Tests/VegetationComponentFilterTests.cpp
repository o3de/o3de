/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "VegetationTest.h"
#include "VegetationMocks.h"

#include <LmbrCentral/Shape/MockShapes.h>

#include <AzTest/AzTest.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/Component/Entity.h>

#include <Source/Components/DistanceBetweenFilterComponent.h>
#include <Source/Components/DistributionFilterComponent.h>
#include <Source/Components/ShapeIntersectionFilterComponent.h>
#include <Source/Components/SurfaceAltitudeFilterComponent.h>
#include <Source/Components/SurfaceMaskDepthFilterComponent.h>
#include <Source/Components/SurfaceMaskFilterComponent.h>
#include <Source/Components/SurfaceSlopeFilterComponent.h>

namespace UnitTest
{
    struct VegetationComponentFilterTests
        : public VegetationComponentTests
    {
        void RegisterComponentDescriptors() override
        {
            m_app.RegisterComponentDescriptor(MockShapeServiceComponent::CreateDescriptor());
            m_app.RegisterComponentDescriptor(MockVegetationAreaServiceComponent::CreateDescriptor());
            m_app.RegisterComponentDescriptor(MockMeshServiceComponent::CreateDescriptor());
        }
    };

    TEST_F(VegetationComponentFilterTests, SurfaceSlopeFilterComponent)
    {
        Vegetation::SurfaceSlopeFilterConfig config;
        config.m_slopeMin = 5.0f;
        config.m_slopeMax = 45.0f;

        Vegetation::SurfaceSlopeFilterComponent* component = nullptr;
        auto entity = CreateEntity(config, &component, [](AZ::Entity* e)
        {
            e->CreateComponent<MockVegetationAreaServiceComponent>();
        });

        // passes
        {
            Vegetation::InstanceData inputInstanceData;
            inputInstanceData.m_normal = AZ::Vector3(0.0f, 0.0f, cosf(AZ::DegToRad((config.m_slopeMin + config.m_slopeMax) / 2)));
            bool result = true;
            Vegetation::FilterRequestBus::EventResult(result, entity->GetId(), &Vegetation::FilterRequestBus::Events::Evaluate, inputInstanceData);
            EXPECT_TRUE(result);
        }

        // blocked
        {
            Vegetation::InstanceData inputInstanceData;
            inputInstanceData.m_normal = AZ::Vector3(0.0f, 0.0f, 0.0f);
            bool result = true;
            Vegetation::FilterRequestBus::EventResult(result, entity->GetId(), &Vegetation::FilterRequestBus::Events::Evaluate, inputInstanceData);
            EXPECT_FALSE(result);
        }
    }

    TEST_F(VegetationComponentFilterTests, SurfaceMaskFilterComponent)
    {
        const auto maskValue = AZ_CRC("test_mask", 0x7a16e9ff);

        Vegetation::SurfaceMaskFilterConfig config;
        config.m_inclusiveSurfaceMasks.push_back(maskValue);

        Vegetation::SurfaceMaskFilterComponent* component = nullptr;
        auto entity = CreateEntity(config, &component, [](AZ::Entity* e)
        {
            e->CreateComponent<MockVegetationAreaServiceComponent>();
        });

        Vegetation::InstanceData vegInstance;
        vegInstance.m_masks[maskValue] = 1.0f;

        // passes
        {
            bool result = true;
            Vegetation::FilterRequestBus::EventResult(result, entity->GetId(), &Vegetation::FilterRequestBus::Events::Evaluate, vegInstance);
            EXPECT_TRUE(result);
        }

        // blocked
        {
            entity->Deactivate();
            config.m_inclusiveSurfaceMasks.clear();
            config.m_exclusiveSurfaceMasks.push_back(maskValue);
            component->ReadInConfig(&config);
            entity->Activate();

            bool result = true;
            Vegetation::FilterRequestBus::EventResult(result, entity->GetId(), &Vegetation::FilterRequestBus::Events::Evaluate, vegInstance);
            EXPECT_FALSE(result);
        }
    }

    TEST_F(VegetationComponentFilterTests, SurfaceMaskDepthFilterComponent)
    {
        Vegetation::SurfaceMaskDepthFilterConfig config;
        config.m_lowerDistance = -1000.0f;
        config.m_upperDistance = -0.5f;
        config.m_depthComparisonTags.push_back(SurfaceData::Constants::s_terrainTagCrc);

        Vegetation::SurfaceMaskDepthFilterComponent* component = nullptr;
        auto entity = CreateEntity(config, &component, [](AZ::Entity* e)
        {
            e->CreateComponent<MockVegetationAreaServiceComponent>();
        });

        MockSurfaceHandler mockSurfaceHandler;
        mockSurfaceHandler.m_outPosition = AZ::Vector3::CreateZero();
        mockSurfaceHandler.m_outNormal = AZ::Vector3::CreateAxisZ();
        mockSurfaceHandler.m_outMasks.clear();
        mockSurfaceHandler.m_outMasks[SurfaceData::Constants::s_terrainTagCrc] = 1.0f;

        // passes
        {
            bool result = true;
            Vegetation::InstanceData inputInstanceData;
            inputInstanceData.m_position = AZ::Vector3(0.0f, 0.0f, -5.0f);
            Vegetation::FilterRequestBus::EventResult(result, entity->GetId(), &Vegetation::FilterRequestBus::Events::Evaluate, inputInstanceData);
            EXPECT_TRUE(result);
        }

        // blocked
        {
            bool result = true;
            Vegetation::InstanceData inputInstanceData;
            inputInstanceData.m_position = AZ::Vector3(0.0f, 0.0f, 5.0f);
            Vegetation::FilterRequestBus::EventResult(result, entity->GetId(), &Vegetation::FilterRequestBus::Events::Evaluate, inputInstanceData);
            EXPECT_FALSE(result);
        }

        EXPECT_EQ(2, mockSurfaceHandler.m_count);
    }

    TEST_F(VegetationComponentFilterTests, SurfaceAltitudeFilterComponent)
    {
        MockShape mockShape;
        mockShape.m_aabb = AZ::Aabb::CreateCenterRadius(AZ::Vector3(0.0f, 0.0f, 0.0f), 10.0f);

        Vegetation::SurfaceAltitudeFilterConfig config;
        config.m_altitudeMin = 0.0f;
        config.m_altitudeMax = 10.0f;
        config.m_shapeEntityId = mockShape.m_entity.GetId();

        Vegetation::SurfaceAltitudeFilterComponent* component = nullptr;
        auto entity = CreateEntity(config, &component, [](AZ::Entity* e)
        {
            e->CreateComponent<MockVegetationAreaServiceComponent>();
        });

        // passes
        {
            bool result = true;
            Vegetation::InstanceData inputInstanceData;
            inputInstanceData.m_position = AZ::Vector3(0.0f, 0.0f, 5.0f);
            Vegetation::FilterRequestBus::EventResult(result, entity->GetId(), &Vegetation::FilterRequestBus::Events::Evaluate, inputInstanceData);
            EXPECT_TRUE(result);
        }

        // blocked
        {
            bool result = true;
            Vegetation::InstanceData inputInstanceData;
            inputInstanceData.m_position = AZ::Vector3(0.0f, 0.0f, 15.0f);
            Vegetation::FilterRequestBus::EventResult(result, entity->GetId(), &Vegetation::FilterRequestBus::Events::Evaluate, inputInstanceData);
            EXPECT_FALSE(result);
        }
    }

    TEST_F(VegetationComponentFilterTests, ShapeIntersectionFilterComponent)
    {
        MockShape mockShape;
        mockShape.m_pointInside = false;

        Vegetation::ShapeIntersectionFilterConfig config;
        config.m_shapeEntityId = mockShape.m_entity.GetId();

        Vegetation::ShapeIntersectionFilterComponent* component = nullptr;
        auto entity = CreateEntity(config, &component, [](AZ::Entity* e)
        {
            e->CreateComponent<MockVegetationAreaServiceComponent>();
        });

        bool result = true;
        Vegetation::InstanceData inputInstanceData;
        Vegetation::FilterRequestBus::EventResult(result, entity->GetId(), &Vegetation::FilterRequestBus::Events::Evaluate, inputInstanceData);
        EXPECT_FALSE(result);
        EXPECT_EQ(1, mockShape.m_count);
    }

    TEST_F(VegetationComponentFilterTests, DistanceBetweenFilterComponent)
    {
        Vegetation::DistanceBetweenFilterConfig config;
        config.m_allowOverrides = true;
        config.m_radiusMin = 1.0f;
        config.m_boundMode = Vegetation::BoundMode::Radius;

        Vegetation::DistanceBetweenFilterComponent* component = nullptr;
        auto entity = CreateEntity(config, &component, [](AZ::Entity* e)
        {
            e->CreateComponent<MockVegetationAreaServiceComponent>();
        });

        Vegetation::InstanceData existingInstance;
        existingInstance.m_position = AZ::Vector3(20.0f);
        existingInstance.m_descriptorPtr.reset(aznew Vegetation::Descriptor());
        existingInstance.m_descriptorPtr->m_radiusOverrideEnabled = true;
        existingInstance.m_descriptorPtr->m_radiusMin = 4.0f;

        MockAreaManager mockAreaManager;
        mockAreaManager.m_existingInstances.clear();
        mockAreaManager.m_existingInstances.emplace_back(existingInstance);

        //test that instances overlap (filter fails)
        {
            bool result = true;
            Vegetation::InstanceData inputInstanceData;
            inputInstanceData.m_position = AZ::Vector3(10.0f);
            inputInstanceData.m_descriptorPtr.reset(aznew Vegetation::Descriptor());
            inputInstanceData.m_descriptorPtr->m_radiusOverrideEnabled = true;
            inputInstanceData.m_descriptorPtr->m_radiusMin = 10.0f;
            Vegetation::FilterRequestBus::EventResult(result, entity->GetId(), &Vegetation::FilterRequestBus::Events::Evaluate, inputInstanceData);
            EXPECT_FALSE(result);
            EXPECT_EQ(1, mockAreaManager.m_count);

            result = true;
            inputInstanceData.m_descriptorPtr->m_radiusOverrideEnabled = false; //(use default radius)
            Vegetation::FilterRequestBus::EventResult(result, entity->GetId(), &Vegetation::FilterRequestBus::Events::Evaluate, inputInstanceData);
            EXPECT_TRUE(result);
            EXPECT_EQ(2, mockAreaManager.m_count);
        }

        //test that instances do not overlap (filter passes)
        {
            bool result = true;
            Vegetation::InstanceData inputInstanceData;
            inputInstanceData.m_position = AZ::Vector3(26.0f);
            inputInstanceData.m_descriptorPtr.reset(aznew Vegetation::Descriptor());
            inputInstanceData.m_descriptorPtr->m_radiusOverrideEnabled = true;
            inputInstanceData.m_descriptorPtr->m_radiusMin = 6.0f;
            Vegetation::FilterRequestBus::EventResult(result, entity->GetId(), &Vegetation::FilterRequestBus::Events::Evaluate, inputInstanceData);
            EXPECT_FALSE(result);
            EXPECT_EQ(3, mockAreaManager.m_count);

            result = true;
            inputInstanceData.m_descriptorPtr->m_radiusOverrideEnabled = false; //(use default radius)
            Vegetation::FilterRequestBus::EventResult(result, entity->GetId(), &Vegetation::FilterRequestBus::Events::Evaluate, inputInstanceData);
            EXPECT_TRUE(result);
            EXPECT_EQ(4, mockAreaManager.m_count);
        }
    }

    TEST_F(VegetationComponentFilterTests, DistributionFilterComponent)
    {
        MockGradientRequestHandler mockGradient;
        mockGradient.m_defaultValue = 0.5f;

        Vegetation::DistributionFilterConfig config;
        config.m_filterStage = Vegetation::FilterStage::Default;
        config.m_gradientSampler.m_gradientId = mockGradient.m_entity.GetId();
        config.m_thresholdMax = 0.90f;
        config.m_thresholdMin = 0.10f;

        Vegetation::DistributionFilterComponent* component = nullptr;
        auto entity = CreateEntity(config, &component, [](AZ::Entity* e)
        {
            e->CreateComponent<MockVegetationAreaServiceComponent>();
        });

        {
            bool result = true;
            Vegetation::InstanceData inputInstanceData;
            Vegetation::FilterRequestBus::EventResult(result, entity->GetId(), &Vegetation::FilterRequestBus::Events::Evaluate, inputInstanceData);
            EXPECT_TRUE(result);
            EXPECT_EQ(1, mockGradient.m_count);
        }

        {
            bool result = true;
            Vegetation::InstanceData inputInstanceData;
            mockGradient.m_defaultValue = 0.0f;
            Vegetation::FilterRequestBus::EventResult(result, entity->GetId(), &Vegetation::FilterRequestBus::Events::Evaluate, inputInstanceData);
            EXPECT_FALSE(result);
            EXPECT_EQ(2, mockGradient.m_count);
        }
    }

}
