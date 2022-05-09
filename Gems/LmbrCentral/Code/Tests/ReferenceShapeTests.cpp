/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/Math/Random.h>
#include <AzCore/Memory/MemoryComponent.h>

#include <LmbrCentral/Shape/MockShapes.h>
#include <Shape/ReferenceShapeComponent.h>

namespace UnitTest
{
    class ReferenceComponentTests
        : public AllocatorsFixture
    {
    protected:
        AZ::ComponentApplication m_app;

        void SetUp() override
        {
            AZ::ComponentApplication::Descriptor appDesc;
            appDesc.m_memoryBlocksByteSize = 20 * 1024 * 1024;
            appDesc.m_recordingMode = AZ::Debug::AllocationRecords::RECORD_NO_RECORDS;
            appDesc.m_stackRecordLevels = 20;

            m_app.Create(appDesc);
        }

        void TearDown() override
        {
            m_app.Destroy();
        }

        template<typename Component, typename Configuration>
        AZStd::unique_ptr<AZ::Entity> CreateEntity(const Configuration& config, Component** ppComponent)
        {
            m_app.RegisterComponentDescriptor(Component::CreateDescriptor());

            auto entity = AZStd::make_unique<AZ::Entity>();

            if (ppComponent)
            {
                *ppComponent = entity->CreateComponent<Component>(config);
            }
            else
            {
                entity->CreateComponent<Component>(config);
            }

            entity->Init();
            EXPECT_EQ(AZ::Entity::State::Init, entity->GetState());

            entity->Activate();
            EXPECT_EQ(AZ::Entity::State::Active, entity->GetState());

            return entity;
        }

        template<typename ComponentA, typename ComponentB>
        bool IsComponentCompatible()
        {
            AZ::ComponentDescriptor::DependencyArrayType providedServicesA;
            ComponentA::GetProvidedServices(providedServicesA);

            AZ::ComponentDescriptor::DependencyArrayType incompatibleServicesB;
            ComponentB::GetIncompatibleServices(incompatibleServicesB);

            for (auto providedServiceA : providedServicesA)
            {
                for (auto incompatibleServiceB : incompatibleServicesB)
                {
                    if (providedServiceA == incompatibleServiceB)
                    {
                        return false;
                    }
                }
            }
            return true;
        }

        template<typename ComponentA, typename ComponentB>
        bool AreComponentsCompatible()
        {
            return IsComponentCompatible<ComponentA, ComponentB>() && IsComponentCompatible<ComponentB, ComponentA>();
        }
    };

    TEST_F(ReferenceComponentTests, VerifyCompatibility)
    {
        EXPECT_FALSE((AreComponentsCompatible<LmbrCentral::ReferenceShapeComponent, LmbrCentral::ReferenceShapeComponent>()));
    }

    TEST_F(ReferenceComponentTests, ReferenceShapeComponent_WithValidReference)
    {
        UnitTest::MockShape testShape;

        LmbrCentral::ReferenceShapeConfig config;
        config.m_shapeEntityId = testShape.m_entity.GetId();

        LmbrCentral::ReferenceShapeComponent* component;
        auto entity = CreateEntity(config, &component);

        AZ::RandomDistributionType randomDistribution = AZ::RandomDistributionType::Normal;
        AZ::Vector3 randPos = AZ::Vector3::CreateOne();
        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            randPos, entity->GetId(), &LmbrCentral::ShapeComponentRequestsBus::Events::GenerateRandomPointInside, randomDistribution);
        EXPECT_EQ(AZ::Vector3::CreateZero(), randPos);

        testShape.m_aabb = AZ::Aabb::CreateFromPoint(AZ::Vector3(1.0f, 21.0f, 31.0f));
        AZ::Aabb resultAABB;
        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            resultAABB, entity->GetId(), &LmbrCentral::ShapeComponentRequestsBus::Events::GetEncompassingAabb);
        EXPECT_EQ(testShape.m_aabb, resultAABB);

        AZ::Crc32 resultCRC = {};
        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            resultCRC, entity->GetId(), &LmbrCentral::ShapeComponentRequestsBus::Events::GetShapeType);
        EXPECT_EQ(AZ_CRC("TestShape", 0x856ca50c), resultCRC);

        testShape.m_localBounds = AZ::Aabb::CreateFromPoint(AZ::Vector3(1.0f, 21.0f, 31.0f));
        testShape.m_localTransform = AZ::Transform::CreateTranslation(testShape.m_localBounds.GetCenter());
        AZ::Transform resultTransform = AZ::Transform::CreateIdentity();
        AZ::Aabb resultBounds = AZ::Aabb::CreateNull();
        LmbrCentral::ShapeComponentRequestsBus::Event(
            entity->GetId(), &LmbrCentral::ShapeComponentRequestsBus::Events::GetTransformAndLocalBounds, resultTransform, resultBounds);
        EXPECT_EQ(testShape.m_localTransform, resultTransform);
        EXPECT_EQ(testShape.m_localBounds, resultBounds);

        testShape.m_pointInside = true;
        bool resultPointInside = false;
        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            resultPointInside, entity->GetId(), &LmbrCentral::ShapeComponentRequestsBus::Events::IsPointInside, AZ::Vector3::CreateZero());
        EXPECT_EQ(testShape.m_pointInside, resultPointInside);

        testShape.m_distanceSquaredFromPoint = 456.0f;
        float resultdistanceSquaredFromPoint = 0;
        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            resultdistanceSquaredFromPoint, entity->GetId(), &LmbrCentral::ShapeComponentRequestsBus::Events::DistanceSquaredFromPoint,
            AZ::Vector3::CreateZero());
        EXPECT_EQ(testShape.m_distanceSquaredFromPoint, resultdistanceSquaredFromPoint);

        testShape.m_intersectRay = false;
        bool resultIntersectRay = false;
        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            resultIntersectRay, entity->GetId(), &LmbrCentral::ShapeComponentRequestsBus::Events::IntersectRay, AZ::Vector3::CreateZero(),
            AZ::Vector3::CreateZero(), 0.0f);
        EXPECT_TRUE(testShape.m_intersectRay == resultIntersectRay);
    }

    TEST_F(ReferenceComponentTests, ReferenceShapeComponent_WithInvalidReference)
    {
        LmbrCentral::ReferenceShapeConfig config;
        config.m_shapeEntityId = AZ::EntityId();

        LmbrCentral::ReferenceShapeComponent* component;
        auto entity = CreateEntity(config, &component);

        AZ::RandomDistributionType randomDistribution = AZ::RandomDistributionType::Normal;
        AZ::Vector3 randPos = AZ::Vector3::CreateOne();
        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            randPos, entity->GetId(), &LmbrCentral::ShapeComponentRequestsBus::Events::GenerateRandomPointInside, randomDistribution);
        EXPECT_EQ(randPos, AZ::Vector3::CreateZero());

        AZ::Aabb resultAABB;
        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            resultAABB, entity->GetId(), &LmbrCentral::ShapeComponentRequestsBus::Events::GetEncompassingAabb);
        EXPECT_EQ(resultAABB, AZ::Aabb::CreateNull());

        AZ::Crc32 resultCRC;
        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            resultCRC, entity->GetId(), &LmbrCentral::ShapeComponentRequestsBus::Events::GetShapeType);
        EXPECT_EQ(resultCRC, AZ::Crc32(AZ::u32(0)));

        AZ::Transform resultTransform;
        AZ::Aabb resultBounds;
        LmbrCentral::ShapeComponentRequestsBus::Event(
            entity->GetId(), &LmbrCentral::ShapeComponentRequestsBus::Events::GetTransformAndLocalBounds, resultTransform, resultBounds);
        EXPECT_EQ(resultTransform, AZ::Transform::CreateIdentity());
        EXPECT_EQ(resultBounds, AZ::Aabb::CreateNull());

        bool resultPointInside = true;
        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            resultPointInside, entity->GetId(), &LmbrCentral::ShapeComponentRequestsBus::Events::IsPointInside, AZ::Vector3::CreateZero());
        EXPECT_EQ(resultPointInside, false);

        float resultdistanceSquaredFromPoint = 0;
        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            resultdistanceSquaredFromPoint, entity->GetId(), &LmbrCentral::ShapeComponentRequestsBus::Events::DistanceSquaredFromPoint,
            AZ::Vector3::CreateZero());
        EXPECT_EQ(resultdistanceSquaredFromPoint, FLT_MAX);

        bool resultIntersectRay = true;
        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            resultIntersectRay, entity->GetId(), &LmbrCentral::ShapeComponentRequestsBus::Events::IntersectRay, AZ::Vector3::CreateZero(),
            AZ::Vector3::CreateZero(), 0.0f);
        EXPECT_EQ(resultIntersectRay, false);
    }
} // namespace UnitTest
