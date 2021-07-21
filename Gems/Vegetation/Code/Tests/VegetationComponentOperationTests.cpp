/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "VegetationTest.h"
#include "VegetationMocks.h"

#include <AzTest/AzTest.h>
#include <AzCore/Component/Entity.h>

#include <Source/Components/AreaBlenderComponent.h>
#include <Source/Components/BlockerComponent.h>
#include <Source/Components/MeshBlockerComponent.h>
#include <Source/Components/SpawnerComponent.h>
#include <Source/InstanceSystemComponent.h>
#include <Source/DebugSystemComponent.h>
#include <Source/Debugger/AreaDebugComponent.h>
#include <Vegetation/EmptyInstanceSpawner.h>

#include <AzCore/Component/TickBus.h>

namespace UnitTest
{
    struct MockDescriptorProvider
        : public Vegetation::DescriptorProviderRequestBus::Handler
    {
        AZStd::vector<Vegetation::DescriptorPtr> m_descriptors;
        MockMeshAsset mockMeshAssetData;

        MockDescriptorProvider(size_t count)
        {
            for (size_t i = 0; i < count; ++i)
            {
                m_descriptors.emplace_back(CreateDescriptor(i));
            }
        }

        Vegetation::DescriptorPtr CreateDescriptor([[maybe_unused]] size_t id)
        {
            Vegetation::Descriptor descriptor;

            auto instanceSpawner = AZStd::make_shared<Vegetation::EmptyInstanceSpawner>();
            descriptor.SetInstanceSpawner(instanceSpawner);

            Vegetation::DescriptorPtr descriptorPtr;
            Vegetation::InstanceSystemRequestBus::BroadcastResult(descriptorPtr, &Vegetation::InstanceSystemRequestBus::Events::RegisterUniqueDescriptor, descriptor);
            return descriptorPtr;
        }

        void Clear()
        {
            for (auto& descriptorPtr : m_descriptors)
            {
                Vegetation::InstanceSystemRequestBus::Broadcast(&Vegetation::InstanceSystemRequestBus::Events::ReleaseUniqueDescriptor, descriptorPtr);
            }
            m_descriptors.clear();
        }

        void GetDescriptors(Vegetation::DescriptorPtrVec& descriptors) const override
        {
            descriptors = m_descriptors;
        }
    };

    struct VegetationComponentOperationTests
        : public VegetationComponentTests
    {
        void RegisterComponentDescriptors() override
        {
            m_app.RegisterComponentDescriptor(MockShapeServiceComponent::CreateDescriptor());
            m_app.RegisterComponentDescriptor(MockVegetationAreaServiceComponent::CreateDescriptor());
            m_app.RegisterComponentDescriptor(MockMeshServiceComponent::CreateDescriptor());
            m_app.RegisterComponentDescriptor(Vegetation::InstanceSystemComponent::CreateDescriptor());
            m_app.RegisterComponentDescriptor(Vegetation::DebugSystemComponent::CreateDescriptor());
            m_app.RegisterComponentDescriptor(Vegetation::AreaDebugComponent::CreateDescriptor());
        }

        void BasicAreaTests(const AZ::EntityId& areaId)
        {
            AZ::u32 priority = std::numeric_limits<AZ::u32>::max();
            Vegetation::AreaInfoBus::EventResult(priority, areaId, &Vegetation::AreaInfoBus::Events::GetPriority);

            EXPECT_NE(priority, std::numeric_limits<AZ::u32>::max());

            AZ::u32 layer = std::numeric_limits<AZ::u32>::max();
            Vegetation::AreaInfoBus::EventResult(layer, areaId, &Vegetation::AreaInfoBus::Events::GetLayer);

            EXPECT_NE(layer, std::numeric_limits<AZ::u32>::max());

            auto aabb = AZ::Aabb::CreateNull();
            Vegetation::AreaInfoBus::EventResult(aabb, areaId, &Vegetation::AreaInfoBus::Events::GetEncompassingAabb);

            EXPECT_TRUE(aabb.IsValid());

            // with area not connected
            {
                Vegetation::AreaNotificationBus::Event(areaId, &Vegetation::AreaNotificationBus::Events::OnAreaDisconnect);

                size_t numHandlers = Vegetation::AreaRequestBus::GetNumOfEventHandlers(areaId);

                EXPECT_EQ(numHandlers, 0);
            }

            // now with area connected
            {
                Vegetation::AreaNotificationBus::Event(areaId, &Vegetation::AreaNotificationBus::Events::OnAreaConnect);

                size_t numHandlers = Vegetation::AreaRequestBus::GetNumOfEventHandlers(areaId);

                EXPECT_EQ(numHandlers, 1);
            }
        }

        void ConnectToAreaBuses(AZ::Entity& entity)
        {
            if (!m_connected)
            {
                entity.Deactivate();
                m_mockMeshRequestBus.BusConnect(entity.GetId());
                m_mockTransformBus.BusConnect(entity.GetId());
                m_mockShapeBus.BusConnect(entity.GetId());
                entity.Activate();

                BasicAreaTests(entity.GetId());
                m_connected = true;
            }

        }

        void ReleaseFromAreaBuses()
        {
            if (m_connected)
            {
                m_mockMeshRequestBus.m_GetMeshAssetOutput.Reset();
                m_mockMeshRequestBus.BusDisconnect();
                m_mockTransformBus.BusDisconnect();
                m_mockShapeBus.BusDisconnect();
                m_connected = false;
            }
        }

        AZ::Data::Asset<MockMeshAsset> CreateMockMeshAsset()
        {
            m_mockMeshAssetData = new MockMeshAsset();
            AZ::Data::Asset<MockMeshAsset> mockAsset(m_mockMeshAssetData, AZ::Data::AssetLoadBehavior::Default);
            return mockAsset;
        }

        void DestroyMockMeshAsset(AZ::Data::Asset<MockMeshAsset>& mockAsset)
        {
            if (m_mockMeshAssetData)
            {
                mockAsset.Reset();
                delete m_mockMeshAssetData;
                m_mockMeshAssetData = nullptr;
            }
        }

        AZStd::unique_ptr<AZ::Entity> CreateMockMeshEntity(const AZ::Data::Asset<MockMeshAsset>& mockAsset, AZ::Vector3 position,
            AZ::Vector3 aabbMin, AZ::Vector3 aabbMax, float meshPercentMin, float meshPercentMax)
        {
            m_mockMeshRequestBus.m_GetMeshAssetOutput = mockAsset;
            m_mockMeshRequestBus.m_GetWorldBoundsOutput = AZ::Aabb::CreateFromMinMax(aabbMin, aabbMax);
            m_mockMeshRequestBus.m_GetVisibilityOutput = true;

            m_mockTransformBus.m_GetWorldTMOutput = AZ::Transform::CreateTranslation(position);

            Vegetation::MeshBlockerConfig config;
            config.m_blockWhenInvisible = true;
            config.m_priority = 2;
            config.m_meshHeightPercentMin = meshPercentMin;
            config.m_meshHeightPercentMax = meshPercentMax;

            Vegetation::MeshBlockerComponent* component = nullptr;
            auto entity = CreateEntity(config, &component, [](AZ::Entity* e)
            {
                e->CreateComponent<MockMeshServiceComponent>();
            });

            return entity;
        }

        void TestMeshBlockerPoint(AZStd::unique_ptr<AZ::Entity>& meshBlockerEntity, AZ::Vector3 testPoint, int numPointsBlocked)
        {
            AreaBusScope scope(*this, *meshBlockerEntity.get());

            Vegetation::AreaNotificationBus::Event(meshBlockerEntity->GetId(), &Vegetation::AreaNotificationBus::Events::OnAreaConnect);

            bool prepared = false;
            Vegetation::EntityIdStack idStack;
            Vegetation::AreaRequestBus::EventResult(prepared, meshBlockerEntity->GetId(), &Vegetation::AreaRequestBus::Events::PrepareToClaim, idStack);
            EXPECT_TRUE(prepared);

            Vegetation::ClaimContext context = CreateContext<1>({ testPoint });
            Vegetation::AreaRequestBus::Event(meshBlockerEntity->GetId(), &Vegetation::AreaRequestBus::Events::ClaimPositions, idStack, context);
            EXPECT_EQ(numPointsBlocked, m_createdCallbackCount);

            Vegetation::AreaNotificationBus::Event(meshBlockerEntity->GetId(), &Vegetation::AreaNotificationBus::Events::OnAreaDisconnect);
        }

        struct AreaBusScope
        {
            VegetationComponentOperationTests& m_block;

            AreaBusScope(VegetationComponentOperationTests& block, AZ::Entity& entity)
                : m_block(block)
            {
                m_block.ConnectToAreaBuses(entity);
            }

            ~AreaBusScope()
            {
                m_block.ReleaseFromAreaBuses();
            }
        };

        bool m_connected = false;
        MockMeshRequestBus m_mockMeshRequestBus;
        MockTransformBus m_mockTransformBus;
        MockShapeComponentNotificationsBus m_mockShapeBus;
        MockMeshAsset* m_mockMeshAssetData;
    };
#if AZ_TRAIT_DISABLE_FAILED_VEGETATION_TESTS
    TEST_F(VegetationComponentOperationTests, DISABLED_MeshBlockerComponent)
#else
    TEST_F(VegetationComponentOperationTests, MeshBlockerComponent)
#endif // AZ_TRAIT_DISABLE_FAILED_VEGETATION_TESTS
    {
        // Create a mock mesh blocker at (0, 0, 0) that extends from (-1, -1, -1) to (1, 1, 1)
        auto mockAsset = CreateMockMeshAsset();
        auto entity = CreateMockMeshEntity(mockAsset, AZ::Vector3::CreateZero(),
            AZ::Vector3(-1.0f), AZ::Vector3(1.0f), 0.0f, 1.0f);

        // Test the point at (0, 0, 0).  It should be blocked.
        const int numPointsBlocked = 1;
        TestMeshBlockerPoint(entity, AZ::Vector3::CreateZero(), numPointsBlocked);

        // Have to destroy the entity here before we destroy the mock mesh asset
        DestroyEntity(entity.release());

        DestroyMockMeshAsset(mockAsset);
    }

    TEST_F(VegetationComponentOperationTests, LY96037_MeshBlockerIntersectionShouldUseZAxis)
    {
        // Create a mock mesh blocker at (0, 0, 0) that extends from (-1, -1, -1) to (1, 10, 1)
        auto mockAsset = CreateMockMeshAsset();
        auto entity = CreateMockMeshEntity(mockAsset, AZ::Vector3::CreateZero(),
            AZ::Vector3(-1.0f, -1.0f, -1.0f), AZ::Vector3(1.0f, 10.0f, 1.0f),
            0.0f, 1.0f);

        // Test the point at (0.5, 0.5, 2.0).  It should *not* be blocked.  
        // Bug LY96037 was that the Y axis is used for height instead of Z, which would cause the point to be blocked.
        // That would make this test fail.
        const int numPointsBlocked = 0;
        TestMeshBlockerPoint(entity, AZ::Vector3(0.5f, 0.5f, 2.0f), numPointsBlocked);

        // Have to destroy the entity here before we destroy the mock mesh asset
        DestroyEntity(entity.release());

        DestroyMockMeshAsset(mockAsset);
    }

#if AZ_TRAIT_DISABLE_FAILED_VEGETATION_TESTS
    TEST_F(VegetationComponentOperationTests, DISABLED_LY96068_MeshBlockerHandlesChangedClaimPoints)
#else
    TEST_F(VegetationComponentOperationTests, LY96068_MeshBlockerHandlesChangedClaimPoints)
#endif // AZ_TRAIT_DISABLE_FAILED_VEGETATION_TESTS
    {
        // Create a mock mesh blocker at (0, 0, 0) that extends from (-1, -1, -1) to (1, 1, 1)
        auto mockAsset = CreateMockMeshAsset();
        auto entity = CreateMockMeshEntity(mockAsset, AZ::Vector3::CreateZero(),
            AZ::Vector3(-1.0f, -1.0f, -1.0f), AZ::Vector3(1.0f, 1.0f, 1.0f),
            0.0f, 1.0f);

        {
            // Putting this into scope so that the AreaBus disconnects before we do any destruction of entities/assets at the end
            AreaBusScope scope(*this, *entity.get());

            Vegetation::AreaNotificationBus::Event(entity->GetId(), &Vegetation::AreaNotificationBus::Events::OnAreaConnect);

            bool prepared = false;
            Vegetation::EntityIdStack idStack;
            Vegetation::AreaRequestBus::EventResult(prepared, entity->GetId(), &Vegetation::AreaRequestBus::Events::PrepareToClaim, idStack);
            EXPECT_TRUE(prepared);

            // Create two different contexts that "reuse" the same claim handle for different points.
            // The first one has a point at (0, 0, 0) that will be successfully blocked.
            // The second one has a point at (2, 2, 2) that should *not* be successfully blocked.
            // Bug LY96068 is that claim handles that change location don't refresh correctly in the Mesh Blocker component.
            AZ::Vector3 claimPosition1 = AZ::Vector3::CreateZero();
            AZ::Vector3 claimPosition2 = AZ::Vector3(2.0f);
            Vegetation::ClaimContext context = CreateContext<1>({ claimPosition1 });
            Vegetation::ClaimContext contextWithReusedHandle = CreateContext<1>({ claimPosition2 });
            contextWithReusedHandle.m_availablePoints[0].m_handle = context.m_availablePoints[0].m_handle;

            // The first time we try with this claim handler, the point should be claimed by the MeshBlocker.
            Vegetation::AreaRequestBus::Event(entity->GetId(), &Vegetation::AreaRequestBus::Events::ClaimPositions, idStack, context);
            EXPECT_EQ(1, m_createdCallbackCount);

            // Clear out our results
            m_createdCallbackCount = 0;

            // Send out a "surface changed" notification, as will as a tick bus tick, to give our mesh blocker a chance to refresh.
            SurfaceData::SurfaceDataSystemNotificationBus::Broadcast(&SurfaceData::SurfaceDataSystemNotificationBus::Events::OnSurfaceChanged, entity->GetId(),
                AZ::Aabb::CreateFromPoint(claimPosition1), AZ::Aabb::CreateFromPoint(claimPosition2));
            AZ::TickBus::Broadcast(&AZ::TickBus::Events::OnTick, 0.f, AZ::ScriptTimePoint{});

            // Try claiming again, this time with the same claim handle, but a different location.
            // This should *not* be claimed by the MeshBlocker.
            Vegetation::AreaRequestBus::Event(entity->GetId(), &Vegetation::AreaRequestBus::Events::ClaimPositions, idStack, contextWithReusedHandle);
            EXPECT_EQ(0, m_createdCallbackCount);

            Vegetation::AreaNotificationBus::Event(entity->GetId(), &Vegetation::AreaNotificationBus::Events::OnAreaDisconnect);
        }

        // Have to destroy the entity here before we destroy the mock mesh asset
        DestroyEntity(entity.release());

        DestroyMockMeshAsset(mockAsset);
    }

    TEST_F(VegetationComponentOperationTests, SpawnerComponent)
    {
        m_mockShapeBus.m_aabb = AZ::Aabb::CreateCenterRadius(AZ::Vector3::CreateZero(), AZ::Constants::FloatMax);

        //need dummy system component to track instance and task stats
        Vegetation::InstanceSystemConfig instanceSystemConfig;
        Vegetation::InstanceSystemComponent* instanceSystemComponent = nullptr;
        auto instanceSystemEntity = CreateEntity(instanceSystemConfig, &instanceSystemComponent, [](AZ::Entity* e)
        {
            e->CreateComponent<Vegetation::DebugSystemComponent>();
        });

        //need spawner to generate instances
        Vegetation::SpawnerConfig config;
        Vegetation::SpawnerComponent* component = nullptr;
        auto entity = CreateEntity(config, &component, [](AZ::Entity* e)
        {
            e->CreateComponent<MockShapeServiceComponent>();
        });

        AreaBusScope scope(*this, *entity.get());

        //need mock descriptor provider to give spawner content to generate
        MockDescriptorProvider mockDescriptorProviderBus(8);
        mockDescriptorProviderBus.BusConnect(entity->GetId());

        //connect the spawner for claim requests
        Vegetation::AreaNotificationBus::Event(entity->GetId(), &Vegetation::AreaNotificationBus::Events::OnAreaConnect);

        bool prepared = false;
        Vegetation::EntityIdStack idStack;
        Vegetation::AreaRequestBus::EventResult(prepared, entity->GetId(), &Vegetation::AreaRequestBus::Events::PrepareToClaim, idStack);
        EXPECT_TRUE(prepared);

        //spawn 32 instances
        Vegetation::ClaimContext context = CreateContext<32>({ AZ::Vector3(0, 0, 0) });
        const Vegetation::ClaimHandle theClaimHandle = context.m_availablePoints[0].m_handle;
        Vegetation::AreaRequestBus::Event(entity->GetId(), &Vegetation::AreaRequestBus::Events::ClaimPositions, idStack, context);

        AZ::u32 createTaskCount = 0;
        Vegetation::InstanceSystemStatsRequestBus::BroadcastResult(createTaskCount, &Vegetation::InstanceSystemStatsRequestBus::Events::GetCreateTaskCount);
        EXPECT_EQ(createTaskCount, 32);

        //destroy the first instance
        Vegetation::AreaRequestBus::Event(entity->GetId(), &Vegetation::AreaRequestBus::Events::UnclaimPosition, theClaimHandle);

        AZ::u32 destroyTaskCount = 0;
        Vegetation::InstanceSystemStatsRequestBus::BroadcastResult(destroyTaskCount, &Vegetation::InstanceSystemStatsRequestBus::Events::GetDestroyTaskCount);
        EXPECT_EQ(destroyTaskCount, 1);

        //disconnect the spawner from claim requests
        Vegetation::AreaNotificationBus::Event(entity->GetId(), &Vegetation::AreaNotificationBus::Events::OnAreaDisconnect);

        //destroy all instances and queued tasks
        Vegetation::InstanceSystemRequestBus::Broadcast(&Vegetation::InstanceSystemRequestBus::Events::DestroyAllInstances);

        //verify tasks amd instances are cleared
        Vegetation::InstanceSystemStatsRequestBus::BroadcastResult(createTaskCount, &Vegetation::InstanceSystemStatsRequestBus::Events::GetCreateTaskCount);
        EXPECT_EQ(createTaskCount, 0);

        Vegetation::InstanceSystemStatsRequestBus::BroadcastResult(destroyTaskCount, &Vegetation::InstanceSystemStatsRequestBus::Events::GetDestroyTaskCount);
        EXPECT_EQ(destroyTaskCount, 0);

        //note: no instances were created because the tick bus did not execute and the test does not setup required engine and renderer systems
        AZ::u32 instanceCount = 0;
        Vegetation::InstanceSystemStatsRequestBus::BroadcastResult(instanceCount, &Vegetation::InstanceSystemStatsRequestBus::Events::GetInstanceCount);
        EXPECT_EQ(instanceCount, 0);

        mockDescriptorProviderBus.Clear();
        mockDescriptorProviderBus.BusDisconnect();
    }

    TEST_F(VegetationComponentOperationTests, AreaBlenderComponent)
    {
        auto entityBlocker = CreateEntity<Vegetation::BlockerComponent>(Vegetation::BlockerConfig(), nullptr, [](AZ::Entity* e)
        {
            e->CreateComponent<MockShapeServiceComponent>();
        });

        MockMeshRequestBus mockMeshRequestBusForBlocker;
        mockMeshRequestBusForBlocker.m_GetWorldBoundsOutput = AZ::Aabb::CreateCenterRadius(AZ::Vector3::CreateZero(), AZ::Constants::FloatMax);
        mockMeshRequestBusForBlocker.m_GetVisibilityOutput = true;
        mockMeshRequestBusForBlocker.BusConnect(entityBlocker->GetId());

        MockTransformBus mockTransformBusForBlocker;
        mockTransformBusForBlocker.m_GetWorldTMOutput = AZ::Transform::CreateTranslation(AZ::Vector3::CreateZero());
        mockMeshRequestBusForBlocker.BusConnect(entityBlocker->GetId());

        MockShapeComponentNotificationsBus mockShapeBusForBlocker;
        mockShapeBusForBlocker.m_aabb = AZ::Aabb::CreateCenterRadius(AZ::Vector3::CreateZero(), AZ::Constants::FloatMax);
        mockMeshRequestBusForBlocker.BusConnect(entityBlocker->GetId());

        Vegetation::AreaBlenderConfig config;
        config.m_vegetationAreaIds.push_back(entityBlocker->GetId());

        Vegetation::AreaBlenderComponent* component = nullptr;
        auto entity = CreateEntity(config, &component, [](AZ::Entity* e)
        {
            e->CreateComponent<MockShapeServiceComponent>();
        });

        AreaBusScope scope(*this, *entity.get());

        Vegetation::AreaNotificationBus::Event(entity->GetId(), &Vegetation::AreaNotificationBus::Events::OnAreaConnect);

        bool prepared = false;
        Vegetation::EntityIdStack idStack;
        Vegetation::AreaRequestBus::EventResult(prepared, entity->GetId(), &Vegetation::AreaRequestBus::Events::PrepareToClaim, idStack);
        EXPECT_TRUE(prepared);

        Vegetation::ClaimContext context = CreateContext<32>({ AZ::Vector3(0, 0, 0) });
        const auto previousPointsCount = context.m_availablePoints.size();
        Vegetation::AreaRequestBus::Event(entity->GetId(), &Vegetation::AreaRequestBus::Events::ClaimPositions, idStack, context);
        EXPECT_NE(previousPointsCount, context.m_availablePoints.size());

        Vegetation::AreaNotificationBus::Event(entity->GetId(), &Vegetation::AreaNotificationBus::Events::OnAreaDisconnect);

        mockMeshRequestBusForBlocker.BusDisconnect();
        mockMeshRequestBusForBlocker.BusDisconnect();
        mockMeshRequestBusForBlocker.BusDisconnect();
    }

    TEST_F(VegetationComponentOperationTests, BlockerComponent)
    {
        m_mockMeshRequestBus.m_GetWorldBoundsOutput = AZ::Aabb::CreateCenterRadius(AZ::Vector3::CreateZero(), AZ::Constants::FloatMax);
        m_mockMeshRequestBus.m_GetVisibilityOutput = true;

        m_mockTransformBus.m_GetWorldTMOutput = AZ::Transform::CreateTranslation(AZ::Vector3::CreateZero());

        m_mockShapeBus.m_aabb = AZ::Aabb::CreateCenterRadius(AZ::Vector3::CreateZero(), AZ::Constants::FloatMax);

        Vegetation::BlockerConfig config;
        config.m_inheritBehavior = false;

        Vegetation::BlockerComponent* component = nullptr;
        auto entity = CreateEntity(config, &component, [](AZ::Entity* e)
        {
            e->CreateComponent<MockShapeServiceComponent>();
        });

        AreaBusScope scope(*this, *entity.get());

        Vegetation::AreaNotificationBus::Event(entity->GetId(), &Vegetation::AreaNotificationBus::Events::OnAreaConnect);

        bool prepared = false;
        Vegetation::EntityIdStack idStack;
        Vegetation::AreaRequestBus::EventResult(prepared, entity->GetId(), &Vegetation::AreaRequestBus::Events::PrepareToClaim, idStack);
        EXPECT_TRUE(prepared);

        Vegetation::EntityIdStack stack;
        Vegetation::ClaimContext claimContext = CreateContext<32>({ AZ::Vector3(0, 0, 0) });
        Vegetation::AreaRequestBus::Event(entity->GetId(), &Vegetation::AreaRequestBus::Events::ClaimPositions, idStack, claimContext);
        EXPECT_EQ(32, m_createdCallbackCount);
        EXPECT_TRUE(claimContext.m_availablePoints.empty());

        Vegetation::AreaNotificationBus::Event(entity->GetId(), &Vegetation::AreaNotificationBus::Events::OnAreaDisconnect);
    }

    TEST_F(VegetationComponentOperationTests, AreaDebugComponent)
    {
        m_mockShapeBus.m_aabb = AZ::Aabb::CreateCenterRadius(AZ::Vector3::CreateZero(), AZ::Constants::FloatMax);

        //define input and expected output colors
        const AZ::Color debugColor1(0.1f, 0.2f, 0.3f, 0.4f);
        const AZ::Color debugColor2(0.5f, 0.6f, 0.7f, 0.8f);
        const AZ::Color debugColorProduct(debugColor1 * debugColor2);

        //create spawner with debug component
        Vegetation::SpawnerConfig spawnerConfig;
        Vegetation::SpawnerComponent* spawnerComponent = nullptr;
        auto spawnerEntity = CreateEntity(spawnerConfig, &spawnerComponent, [&debugColor1](AZ::Entity* e)
        {
            Vegetation::AreaDebugConfig areaDebugConfig;
            areaDebugConfig.m_debugColor = debugColor1;
            e->CreateComponent<Vegetation::AreaDebugComponent>(areaDebugConfig);
            e->CreateComponent<MockShapeServiceComponent>();
        });

        AreaBusScope scope(*this, *spawnerEntity.get());

        //need mock descriptpor provider to give spawner content to generate
        MockDescriptorProvider mockDescriptorProviderBus(8);
        mockDescriptorProviderBus.BusConnect(spawnerEntity->GetId());

        //create blender that references spawner with debug component
        Vegetation::AreaBlenderConfig blenderConfig;
        blenderConfig.m_vegetationAreaIds.push_back(spawnerEntity->GetId());
        Vegetation::AreaBlenderComponent* blenderComponent = nullptr;
        auto blenderEntity = CreateEntity(blenderConfig, &blenderComponent, [&debugColor2](AZ::Entity* e)
        {
            Vegetation::AreaDebugConfig areaDebugConfig;
            areaDebugConfig.m_debugColor = debugColor2;
            e->CreateComponent<Vegetation::AreaDebugComponent>(areaDebugConfig);
            e->CreateComponent<MockShapeServiceComponent>();
        });

        //force blender and referenced spawner to prepare for placement, which recomputes blended debug color
        bool prepared = false;
        Vegetation::EntityIdStack idStack;
        Vegetation::AreaNotificationBus::Event(blenderEntity->GetId(), &Vegetation::AreaNotificationBus::Events::OnAreaConnect);
        Vegetation::AreaRequestBus::EventResult(prepared, blenderEntity->GetId(), &Vegetation::AreaRequestBus::Events::PrepareToClaim, idStack);
        Vegetation::AreaNotificationBus::Event(blenderEntity->GetId(), &Vegetation::AreaNotificationBus::Events::OnAreaDisconnect);
        EXPECT_TRUE(prepared);

        //verify expected debug data
        Vegetation::AreaDebugDisplayData areaDebugDisplayData;
        Vegetation::AreaDebugBus::EventResult(areaDebugDisplayData, spawnerEntity->GetId(), &Vegetation::AreaDebugBus::Events::GetBaseDebugDisplayData);
        EXPECT_EQ(areaDebugDisplayData.m_instanceColor, debugColor1);
        Vegetation::AreaDebugBus::EventResult(areaDebugDisplayData, spawnerEntity->GetId(), &Vegetation::AreaDebugBus::Events::GetBlendedDebugDisplayData);
        EXPECT_EQ(areaDebugDisplayData.m_instanceColor, debugColorProduct);

        Vegetation::AreaDebugBus::EventResult(areaDebugDisplayData, blenderEntity->GetId(), &Vegetation::AreaDebugBus::Events::GetBaseDebugDisplayData);
        EXPECT_EQ(areaDebugDisplayData.m_instanceColor, debugColor2);
        Vegetation::AreaDebugBus::EventResult(areaDebugDisplayData, blenderEntity->GetId(), &Vegetation::AreaDebugBus::Events::GetBlendedDebugDisplayData);
        EXPECT_EQ(areaDebugDisplayData.m_instanceColor, debugColor2);

        mockDescriptorProviderBus.Clear();
        mockDescriptorProviderBus.BusDisconnect();
    }

}
