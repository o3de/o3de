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
#include <AzFramework/Application/Application.h>
#include <AzFramework/Spawnable/SpawnableAssetHandler.h>
#include <AzFramework/Spawnable/SpawnableEntitiesManager.h>
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
            m_manager->ProcessQueue();

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

    protected:
        AZ::Data::Asset<AzFramework::Spawnable>* m_spawnableAsset { nullptr };
        AzFramework::SpawnableEntitiesManager* m_manager { nullptr };
        AzFramework::EntitySpawnTicket* m_ticket { nullptr };
        AzFramework::Spawnable* m_spawnable { nullptr };
        TestApplication* m_application { nullptr };
    };

    TEST_F(SpawnableEntitiesManagerTest, SpawnAllEntities_Call_AllEntitiesSpawned)
    {
        static constexpr size_t NumEntities = 4;
        FillSpawnable(NumEntities);

        size_t spawnedEntitiesCount = 0;
        auto callback =
            [&spawnedEntitiesCount](AzFramework::EntitySpawnTicket&, AzFramework::SpawnableConstEntityContainerView entities)
            {
                spawnedEntitiesCount += entities.size();
            };
        m_manager->SpawnAllEntities(*m_ticket, {}, AZStd::move(callback));
        m_manager->ProcessQueue();

        EXPECT_EQ(NumEntities, spawnedEntitiesCount);
    }

    TEST_F(SpawnableEntitiesManagerTest, ListEntities_Call_AllEntitiesAreReported)
    {
        static constexpr size_t NumEntities = 4;
        FillSpawnable(NumEntities);

        bool allValidEntityIds = true;
        size_t spawnedEntitiesCount = 0;
        auto callback = [&allValidEntityIds, &spawnedEntitiesCount]
            (AzFramework::EntitySpawnTicket&, AzFramework::SpawnableConstEntityContainerView entities)
            {
                for (auto&& entity : entities)
                {
                    allValidEntityIds = entity->GetId().IsValid() && allValidEntityIds;
                }
                spawnedEntitiesCount += entities.size();
            };

        m_manager->SpawnAllEntities(*m_ticket);
        m_manager->ListEntities(*m_ticket, AZStd::move(callback));
        m_manager->ProcessQueue();

        EXPECT_TRUE(allValidEntityIds);
        EXPECT_EQ(NumEntities, spawnedEntitiesCount);
    }

    TEST_F(SpawnableEntitiesManagerTest, ListIndicesAndEntities_Call_AllEntitiesAreReportedAndIncrementByOne)
    {
        static constexpr size_t NumEntities = 4;
        FillSpawnable(NumEntities);

        bool allValidEntityIds = true;
        size_t spawnedEntitiesCount = 0;
        auto callback = [&allValidEntityIds, &spawnedEntitiesCount]
            (AzFramework::EntitySpawnTicket&, AzFramework::SpawnableConstIndexEntityContainerView entities)
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
        m_manager->ProcessQueue();

        EXPECT_TRUE(allValidEntityIds);
        EXPECT_EQ(NumEntities, spawnedEntitiesCount);
    }
} // namespace UnitTest
