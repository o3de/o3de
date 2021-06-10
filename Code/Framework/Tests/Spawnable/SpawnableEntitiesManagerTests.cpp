/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/UserSettings/UserSettingsComponent.h>
#include <AzFramework/Application/Application.h>
#include <AzFramework/Spawnable/SpawnableAssetHandler.h>
#include <AzFramework/Spawnable/SpawnableEntitiesManager.h>
#include <AzFramework/Components/TransformComponent.h>
#include <AzTest/AzTest.h>

namespace UnitTest
{
    class TestApplication : public AzFramework::Application
    {
    public:
        // ComponentApplication
        void SetSettingsRegistrySpecializations(AZ::SettingsRegistryInterface::Specializations& specializations) override
        {
            Application::SetSettingsRegistrySpecializations(specializations);
            specializations.Append("test");
            specializations.Append("spawnable");
        }
    };

    class SpawnableEntitiesManagerTest : public AllocatorsFixture
    {
    public:
        void SetUp() override
        {
            AllocatorsFixture::SetUp();

            m_application = new TestApplication();
            AZ::ComponentApplication::Descriptor descriptor;
            m_application->Start(descriptor);
            // Without this, the user settings component would attempt to save on finalize/shutdown. Since the file is
            // shared across the whole engine, if multiple tests are run in parallel, the saving could cause a crash
            // in the unit tests.
            AZ::UserSettingsComponentRequestBus::Broadcast(&AZ::UserSettingsComponentRequests::DisableSaveOnFinalize);
            
            m_spawnable = aznew AzFramework::Spawnable(
                AZ::Data::AssetId::CreateString("{EB2E8A2B-F253-4A90-BBF4-55F2EED786B8}:0"), AZ::Data::AssetData::AssetStatus::Ready);
            m_spawnableAsset = new AZ::Data::Asset<AzFramework::Spawnable>(m_spawnable, AZ::Data::AssetLoadBehavior::Default);
            m_ticket = new AzFramework::EntitySpawnTicket(*m_spawnableAsset);

            auto managerInterface = AzFramework::SpawnableEntitiesInterface::Get();
            m_manager = azrtti_cast<AzFramework::SpawnableEntitiesManager*>(managerInterface);
        }

        void TearDown() override
        {
            delete m_ticket;
            m_ticket = nullptr;
            // One more tick on the spawnable entities manager in order to delete the ticket fully.
            while (m_manager->ProcessQueue(
                       AzFramework::SpawnableEntitiesManager::CommandQueuePriority::High |
                       AzFramework::SpawnableEntitiesManager::CommandQueuePriority::Regular) !=
                   AzFramework::SpawnableEntitiesManager::CommandQueueStatus::NoCommandsLeft)
                ;

            delete m_spawnableAsset;
            m_spawnableAsset = nullptr;
            // This will also delete m_spawnable.

            delete m_application;
            m_application = nullptr;

            AllocatorsFixture::TearDown();
        }

        void FillSpawnable(size_t numElements)
        {
            AzFramework::Spawnable::EntityList& entities = m_spawnable->GetEntities();
            entities.reserve(numElements);
            for (size_t i=0; i<numElements; ++i)
            {
                entities.push_back(AZStd::make_unique<AZ::Entity>());
            }
        }

        void CreateRecursiveHierarchy()
        {
            AzFramework::Spawnable::EntityList& entities = m_spawnable->GetEntities();
            size_t numElements = entities.size();
            AZ::EntityId parent;
            for (size_t i=0; i<numElements; ++i)
            {
                AZStd::unique_ptr<AZ::Entity>& entity = entities[i];
                auto component = entity->CreateComponent<AzFramework::TransformComponent>();
                if (i > 0)
                {
                    component->SetParent(parent);
                }
                parent = entity->GetId();     
            }
        }

        void CreateSingleParent()
        {
            AzFramework::Spawnable::EntityList& entities = m_spawnable->GetEntities();
            size_t numElements = entities.size();
            if (numElements > 0)
            {
                AZ::EntityId parent = entities[0]->GetId();
                for (size_t i = 0; i < numElements; ++i)
                {
                    AZStd::unique_ptr<AZ::Entity>& entity = entities[i];
                    auto component = entity->CreateComponent<AzFramework::TransformComponent>();
                    if (i > 0)
                    {
                        component->SetParent(parent);
                    }
                }
            }
        }

    protected:
        AZ::Data::Asset<AzFramework::Spawnable>* m_spawnableAsset { nullptr };
        AzFramework::SpawnableEntitiesManager* m_manager { nullptr };
        AzFramework::EntitySpawnTicket* m_ticket { nullptr };
        AzFramework::Spawnable* m_spawnable { nullptr };
        TestApplication* m_application { nullptr };
    };

    //
    // SpawnAllEntitities
    //

    TEST_F(SpawnableEntitiesManagerTest, SpawnAllEntities_Call_AllEntitiesSpawned)
    {
        static constexpr size_t NumEntities = 4;
        FillSpawnable(NumEntities);

        size_t spawnedEntitiesCount = 0;
        auto callback =
            [&spawnedEntitiesCount](AzFramework::EntitySpawnTicket::Id, AzFramework::SpawnableConstEntityContainerView entities)
            {
                spawnedEntitiesCount += entities.size();
            };
        AzFramework::SpawnAllEntitiesOptionalArgs optionalArgs;
        optionalArgs.m_completionCallback = AZStd::move(callback);
        m_manager->SpawnAllEntities(*m_ticket, AZStd::move(optionalArgs));
        m_manager->ProcessQueue(AzFramework::SpawnableEntitiesManager::CommandQueuePriority::Regular);

        EXPECT_EQ(NumEntities, spawnedEntitiesCount);
    }

    TEST_F(SpawnableEntitiesManagerTest, SpawnAllEntities_SetParentOnSpawnedEntities_LineageIsPreserved)
    {
        static constexpr size_t NumEntities = 4;
        FillSpawnable(NumEntities);
        CreateRecursiveHierarchy();

        auto callback = [](AzFramework::EntitySpawnTicket::Id, AzFramework::SpawnableConstEntityContainerView entities)
        {
            AZ::EntityId parentId;
            bool isFirst = true;
            for (const AZ::Entity* entity : entities)
            {
                if (!isFirst)
                {
                    auto transform = entity->GetTransform();
                    ASSERT_NE(nullptr, transform);
                    EXPECT_EQ(parentId, transform->GetParentId());
                }
                else
                {
                    isFirst = false;
                }
                parentId = entity->GetId();
            }
        };
        AzFramework::SpawnAllEntitiesOptionalArgs optionalArgs;
        optionalArgs.m_completionCallback = AZStd::move(callback);
        m_manager->SpawnAllEntities(*m_ticket, AZStd::move(optionalArgs));
        m_manager->ProcessQueue(AzFramework::SpawnableEntitiesManager::CommandQueuePriority::Regular);
    }

    TEST_F(SpawnableEntitiesManagerTest, SpawnAllEntities_DeleteTicketBeforeCall_NoCrash)
    {
        {
            AzFramework::EntitySpawnTicket ticket(*m_spawnableAsset);
            m_manager->SpawnAllEntities(ticket);
        }
        m_manager->ProcessQueue(AzFramework::SpawnableEntitiesManager::CommandQueuePriority::Regular);
    }


    //
    // SpawnEntities
    //

    TEST_F(SpawnableEntitiesManagerTest, SpawnEntities_Call_AllEntitiesSpawned)
    {
        static constexpr size_t NumEntities = 4;
        FillSpawnable(NumEntities);

        AZStd::vector<size_t> indices = { 0, 2, 3, 1 };

        size_t spawnedEntitiesCount = 0;
        auto callback = [&spawnedEntitiesCount](AzFramework::EntitySpawnTicket::Id, AzFramework::SpawnableConstEntityContainerView entities)
        {
            spawnedEntitiesCount += entities.size();
        };
        AzFramework::SpawnEntitiesOptionalArgs optionalArgs;
        optionalArgs.m_completionCallback = AZStd::move(callback);
        m_manager->SpawnEntities(*m_ticket, AZStd::move(indices), AZStd::move(optionalArgs));
        m_manager->ProcessQueue(AzFramework::SpawnableEntitiesManager::CommandQueuePriority::Regular);

        EXPECT_EQ(NumEntities, spawnedEntitiesCount);
    }

    TEST_F(SpawnableEntitiesManagerTest, SpawnEntities_SpawnTheSameEntity_AllEntitiesSpawned)
    {
        static constexpr size_t NumEntities = 1;
        FillSpawnable(NumEntities);

        AZStd::vector<size_t> indices = { 0, 0 };

        size_t spawnedEntitiesCount = 0;
        auto callback =
            [&spawnedEntitiesCount](AzFramework::EntitySpawnTicket::Id, AzFramework::SpawnableConstEntityContainerView entities)
        {
            spawnedEntitiesCount += entities.size();
        };
        AzFramework::SpawnEntitiesOptionalArgs optionalArgs;
        optionalArgs.m_completionCallback = AZStd::move(callback);
        m_manager->SpawnEntities(*m_ticket, AZStd::move(indices), AZStd::move(optionalArgs));
        m_manager->ProcessQueue(AzFramework::SpawnableEntitiesManager::CommandQueuePriority::Regular);

        EXPECT_EQ(NumEntities * 2, spawnedEntitiesCount);
    }

    TEST_F(SpawnableEntitiesManagerTest, SpawnEntities_MultipleSpawns_AllEntitiesSpawned)
    {
        static constexpr size_t NumEntities = 4;
        FillSpawnable(NumEntities);

        AZStd::vector<size_t> indices = { 0, 2, 3, 1 };

        size_t spawnedEntitiesCount = 0;
        auto callback =
            [&spawnedEntitiesCount](AzFramework::EntitySpawnTicket::Id, AzFramework::SpawnableConstEntityContainerView entities)
        {
            spawnedEntitiesCount += entities.size();
        };
        AzFramework::SpawnEntitiesOptionalArgs optionalArgs;
        optionalArgs.m_completionCallback = AZStd::move(callback);
        m_manager->SpawnEntities(*m_ticket, indices, optionalArgs);
        m_manager->SpawnEntities(*m_ticket, AZStd::move(indices), AZStd::move(optionalArgs));
        m_manager->ProcessQueue(AzFramework::SpawnableEntitiesManager::CommandQueuePriority::Regular);

        EXPECT_EQ(NumEntities * 2, spawnedEntitiesCount);
    }

    TEST_F(SpawnableEntitiesManagerTest, SpawnEntities_ReferencesAreRemappedForNewBatch_AllPointToLatestParent)
    {
        static constexpr size_t NumEntities = 4;
        FillSpawnable(NumEntities);
        CreateSingleParent();

        AZStd::vector<size_t> indices = { 0, 1, 2, 3 };
        AZStd::vector<AZ::EntityId> parents;

        auto callback = [&parents](AzFramework::EntitySpawnTicket::Id, AzFramework::SpawnableConstEntityContainerView entities)
        {
            AZ::EntityId parent = (*entities.begin())->GetId();
            parents.push_back(parent);
            auto it = entities.begin();
            ++it; // Skip the first as that is the parent.
            for (; it != entities.end(); ++it)
            {
                AZ::TransformInterface* transform = (*it)->GetTransform();
                ASSERT_NE(nullptr, transform);
                ASSERT_EQ(parent, transform->GetParentId());
            }
        };
        AzFramework::SpawnEntitiesOptionalArgs optionalArgs;
        optionalArgs.m_completionCallback = AZStd::move(callback);
        optionalArgs.m_referencePreviouslySpawnedEntities = false;
        m_manager->SpawnEntities(*m_ticket, indices, optionalArgs);
        m_manager->SpawnEntities(*m_ticket, AZStd::move(indices), AZStd::move(optionalArgs));
        m_manager->ProcessQueue(AzFramework::SpawnableEntitiesManager::CommandQueuePriority::Regular);

        EXPECT_NE(parents[0], parents[1]);
    }

    TEST_F(SpawnableEntitiesManagerTest, SpawnEntities_ReferencesAreRemappedForContinuedBatch_AllPointToLatestParent)
    {
        static constexpr size_t NumEntities = 4;
        FillSpawnable(NumEntities);
        CreateSingleParent();

        AZStd::vector<size_t> indices = { 0, 1, 2, 3 };
        AZStd::vector<AZ::EntityId> parents;

        auto callback =
            [&parents](AzFramework::EntitySpawnTicket::Id, AzFramework::SpawnableConstEntityContainerView entities)
        {
            AZ::EntityId parent = (*entities.begin())->GetId();
            parents.push_back(parent);
            auto it = entities.begin();
            ++it; // Skip the first as that is the parent.
            for (; it!=entities.end(); ++it)
            {
                AZ::TransformInterface* transform = (*it)->GetTransform();
                ASSERT_NE(nullptr, transform);
                ASSERT_EQ(parent, transform->GetParentId());
            }
        };
        AzFramework::SpawnEntitiesOptionalArgs optionalArgs;
        optionalArgs.m_completionCallback = AZStd::move(callback);
        optionalArgs.m_referencePreviouslySpawnedEntities = true;
        m_manager->SpawnEntities(*m_ticket, indices, optionalArgs);
        m_manager->SpawnEntities(*m_ticket, AZStd::move(indices), AZStd::move(optionalArgs));
        m_manager->ProcessQueue(AzFramework::SpawnableEntitiesManager::CommandQueuePriority::Regular);

        EXPECT_NE(parents[0], parents[1]);
    }

    TEST_F(SpawnableEntitiesManagerTest, SpawnEntities_ReferencesAreRemappedAcrossBatches_AllPointToLatestParent)
    {
        FillSpawnable(4);
        CreateSingleParent();

        // Spawn a regular batch but with two parents and store the id of the last entity. This will the parent for the next batch.
        AZ::EntityId parent;
        auto getParent = [&parent](AzFramework::EntitySpawnTicket::Id, AzFramework::SpawnableConstEntityContainerView entities)
        {
            ASSERT_NE(entities.begin(), entities.end());
            parent = (*AZStd::prev(entities.end()))->GetId();
        };
        
        AzFramework::SpawnEntitiesOptionalArgs optionalArgsFirstBatch;
        optionalArgsFirstBatch.m_completionCallback = AZStd::move(getParent);
        optionalArgsFirstBatch.m_referencePreviouslySpawnedEntities = true;
        m_manager->SpawnEntities(*m_ticket, {0, 1, 2, 3, 0}, AZStd::move(optionalArgsFirstBatch));

        // Next, spawn all the entities that have a reference to the parent that was just stored.
        auto parentCheck = [&parent](AzFramework::EntitySpawnTicket::Id, AzFramework::SpawnableConstEntityContainerView entities)
        {
            for (auto& it : entities)
            {
                AZ::TransformInterface* transform = it->GetTransform();
                ASSERT_NE(nullptr, transform);
                ASSERT_EQ(parent, transform->GetParentId());
            }
        };
        AzFramework::SpawnEntitiesOptionalArgs optionalArgsSecondBatch;
        optionalArgsSecondBatch.m_completionCallback = AZStd::move(parentCheck);
        optionalArgsSecondBatch.m_referencePreviouslySpawnedEntities = true;
        m_manager->SpawnEntities(*m_ticket, {1, 2, 3}, AZStd::move(optionalArgsSecondBatch));

        m_manager->ProcessQueue(AzFramework::SpawnableEntitiesManager::CommandQueuePriority::Regular);
    }

    TEST_F(SpawnableEntitiesManagerTest, SpawnEntities_DeleteTicketBeforeCall_NoCrash)
    {
        {
            AzFramework::EntitySpawnTicket ticket(*m_spawnableAsset);
            m_manager->SpawnEntities(ticket, {/* Deliberate empty list of indices. */});
        }
        m_manager->ProcessQueue(AzFramework::SpawnableEntitiesManager::CommandQueuePriority::Regular);
    }


    //
    // DespawnAllEntities
    //

    TEST_F(SpawnableEntitiesManagerTest, DespawnAllEntities_DeleteTicketBeforeCall_NoCrash)
    {
        {
            AzFramework::EntitySpawnTicket ticket(*m_spawnableAsset);
            m_manager->DespawnAllEntities(ticket);
        }
        m_manager->ProcessQueue(AzFramework::SpawnableEntitiesManager::CommandQueuePriority::Regular);
    }


    //
    // ReloadSpawnable
    //

    TEST_F(SpawnableEntitiesManagerTest, ReloadSpawnable_DeleteTicketBeforeCall_NoCrash)
    {
        {
            AzFramework::EntitySpawnTicket ticket(*m_spawnableAsset);
            m_manager->ReloadSpawnable(ticket, *m_spawnableAsset);
        }
        m_manager->ProcessQueue(AzFramework::SpawnableEntitiesManager::CommandQueuePriority::Regular);
    }


    //
    // ListEntitities
    //

    TEST_F(SpawnableEntitiesManagerTest, ListEntities_Call_AllEntitiesAreReported)
    {
        static constexpr size_t NumEntities = 4;
        FillSpawnable(NumEntities);

        bool allValidEntityIds = true;
        size_t spawnedEntitiesCount = 0;
        auto callback = [&allValidEntityIds, &spawnedEntitiesCount]
            (AzFramework::EntitySpawnTicket::Id, AzFramework::SpawnableConstEntityContainerView entities)
            {
                for (auto&& entity : entities)
                {
                    allValidEntityIds = entity->GetId().IsValid() && allValidEntityIds;
                }
                spawnedEntitiesCount += entities.size();
            };

        m_manager->SpawnAllEntities(*m_ticket);
        m_manager->ListEntities(*m_ticket, AZStd::move(callback));
        m_manager->ProcessQueue(AzFramework::SpawnableEntitiesManager::CommandQueuePriority::Regular);

        EXPECT_TRUE(allValidEntityIds);
        EXPECT_EQ(NumEntities, spawnedEntitiesCount);
    }

    TEST_F(SpawnableEntitiesManagerTest, ListEntities_DeleteTicketBeforeCall_NoCrash)
    {
        auto callback = [](AzFramework::EntitySpawnTicket::Id, AzFramework::SpawnableConstEntityContainerView) {};

        {
            AzFramework::EntitySpawnTicket ticket(*m_spawnableAsset);
            m_manager->ListEntities(ticket, AZStd::move(callback));
        }
        m_manager->ProcessQueue(AzFramework::SpawnableEntitiesManager::CommandQueuePriority::Regular);
    }


    //
    // ListIndicesAndEntities
    //

    TEST_F(SpawnableEntitiesManagerTest, ListIndicesAndEntities_Call_AllEntitiesAreReportedAndIncrementByOne)
    {
        static constexpr size_t NumEntities = 4;
        FillSpawnable(NumEntities);

        bool allValidEntityIds = true;
        size_t spawnedEntitiesCount = 0;
        auto callback = [&allValidEntityIds, &spawnedEntitiesCount]
            (AzFramework::EntitySpawnTicket::Id, AzFramework::SpawnableConstIndexEntityContainerView entities)
            {
                for (auto&& indexEntityPair : entities)
                {
                    // Since all entities are spawned a single time, the indices should be 0..NumEntities.
                    if (indexEntityPair.GetIndex() == spawnedEntitiesCount)
                    {
                        spawnedEntitiesCount++;
                    }
                    allValidEntityIds = indexEntityPair.GetEntity()->GetId().IsValid() && allValidEntityIds;
                }
            };

        m_manager->SpawnAllEntities(*m_ticket);
        m_manager->ListIndicesAndEntities(*m_ticket, AZStd::move(callback));
        m_manager->ProcessQueue(AzFramework::SpawnableEntitiesManager::CommandQueuePriority::Regular);

        EXPECT_TRUE(allValidEntityIds);
        EXPECT_EQ(NumEntities, spawnedEntitiesCount);
    }

    TEST_F(SpawnableEntitiesManagerTest, ListIndicesAndEntities_DeleteTicketBeforeCall_NoCrash)
    {
        auto callback = [](AzFramework::EntitySpawnTicket::Id, AzFramework::SpawnableConstIndexEntityContainerView) {};

        {
            AzFramework::EntitySpawnTicket ticket(*m_spawnableAsset);
            m_manager->ListIndicesAndEntities(ticket, AZStd::move(callback));
        }
        m_manager->ProcessQueue(AzFramework::SpawnableEntitiesManager::CommandQueuePriority::Regular);
    }


    //
    // ClaimEntities
    //

    TEST_F(SpawnableEntitiesManagerTest, ClaimEntities_DeleteTicketBeforeCall_NoCrash)
    {
        auto callback = [](AzFramework::EntitySpawnTicket::Id, AzFramework::SpawnableEntityContainerView) {};

        {
            AzFramework::EntitySpawnTicket ticket(*m_spawnableAsset);
            m_manager->ClaimEntities(ticket, AZStd::move(callback));
        }
        m_manager->ProcessQueue(AzFramework::SpawnableEntitiesManager::CommandQueuePriority::Regular);
    }


    //
    // Barrier
    //

    TEST_F(SpawnableEntitiesManagerTest, Barrier_DeleteTicketBeforeCall_NoCrash)
    {
        auto callback = [](AzFramework::EntitySpawnTicket::Id) {};

        {
            AzFramework::EntitySpawnTicket ticket(*m_spawnableAsset);
            m_manager->Barrier(ticket, AZStd::move(callback));
        }
        m_manager->ProcessQueue(AzFramework::SpawnableEntitiesManager::CommandQueuePriority::Regular);
    }


    //
    // Misc. - Priority tests
    //

    TEST_F(SpawnableEntitiesManagerTest, Priority_HighBeforeDefault_HigherPriorityCallHappensBeforeDefaultPriorityEvenWhenQueuedLater)
    {
        static constexpr size_t NumEntities = 4;
        FillSpawnable(NumEntities);

        AzFramework::EntitySpawnTicket highPriorityTicket(*m_spawnableAsset);

        size_t callCounter = 1;
        size_t highPriorityCallId = 0;
        size_t defaultPriorityCallId = 0;
        auto highCallback = [&callCounter, &highPriorityCallId]
            (AzFramework::EntitySpawnTicket::Id, AzFramework::SpawnableConstEntityContainerView)
            {
                highPriorityCallId = callCounter++;
            };
        auto defaultCallback = [&callCounter, &defaultPriorityCallId]
            (AzFramework::EntitySpawnTicket::Id, AzFramework::SpawnableConstEntityContainerView)
            {
                defaultPriorityCallId = callCounter++;
            };

        AzFramework::SpawnAllEntitiesOptionalArgs optionalArgs;
        optionalArgs.m_completionCallback = AZStd::move(defaultCallback);
        optionalArgs.m_priority = AzFramework::SpawnablePriority_Default;
        m_manager->SpawnAllEntities(*m_ticket, AZStd::move(optionalArgs));

        AzFramework::SpawnAllEntitiesOptionalArgs highPriortyOptionalArgs;
        highPriortyOptionalArgs.m_completionCallback = AZStd::move(highCallback);
        highPriortyOptionalArgs.m_priority = AzFramework::SpawnablePriority_High;
        m_manager->SpawnAllEntities(highPriorityTicket, AZStd::move(highPriortyOptionalArgs));

        m_manager->ProcessQueue(
            AzFramework::SpawnableEntitiesManager::CommandQueuePriority::High |
            AzFramework::SpawnableEntitiesManager::CommandQueuePriority::Regular);

        EXPECT_LT(highPriorityCallId, defaultPriorityCallId);
    }

    TEST_F(SpawnableEntitiesManagerTest, Priority_SameTicket_DefaultPriorityCallHappensBeforeHighPriority)
    {
        static constexpr size_t NumEntities = 4;
        FillSpawnable(NumEntities);

        size_t callCounter = 1;
        size_t highPriorityCallId = 0;
        size_t defaultPriorityCallId = 0;
        auto highCallback =
            [&callCounter, &highPriorityCallId](AzFramework::EntitySpawnTicket::Id, AzFramework::SpawnableConstEntityContainerView)
            {
                highPriorityCallId = callCounter++;
            };
        auto defaultCallback =
            [&callCounter, &defaultPriorityCallId](AzFramework::EntitySpawnTicket::Id, AzFramework::SpawnableConstEntityContainerView)
            {
                defaultPriorityCallId = callCounter++;
            };

        AzFramework::SpawnAllEntitiesOptionalArgs optionalArgs;
        optionalArgs.m_completionCallback = AZStd::move(defaultCallback);
        optionalArgs.m_priority = AzFramework::SpawnablePriority_Default;
        m_manager->SpawnAllEntities(*m_ticket, AZStd::move(optionalArgs));

        AzFramework::SpawnAllEntitiesOptionalArgs highPriortyOptionalArgs;
        highPriortyOptionalArgs.m_completionCallback = AZStd::move(highCallback);
        highPriortyOptionalArgs.m_priority = AzFramework::SpawnablePriority_High;
        m_manager->SpawnAllEntities(*m_ticket, AZStd::move(highPriortyOptionalArgs));

        m_manager->ProcessQueue(
            AzFramework::SpawnableEntitiesManager::CommandQueuePriority::High |
            AzFramework::SpawnableEntitiesManager::CommandQueuePriority::Regular);
        // Run a second time as the high priority task will be pending at this point.
        m_manager->ProcessQueue(
            AzFramework::SpawnableEntitiesManager::CommandQueuePriority::High |
            AzFramework::SpawnableEntitiesManager::CommandQueuePriority::Regular);

        EXPECT_LT(defaultPriorityCallId, highPriorityCallId);
    }
} // namespace UnitTest
