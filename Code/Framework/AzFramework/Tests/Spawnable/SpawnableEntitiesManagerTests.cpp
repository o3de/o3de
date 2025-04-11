/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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

    // Test component that has a reference to a different entity for use in validating per-instance entity id fixups.
    class ComponentWithEntityReference : public AZ::Component
    {
    public:
        AZ_COMPONENT(ComponentWithEntityReference, "{CF5FDE59-86E5-40B6-9272-BBC1C4AFD061}");

        void Activate() override
        {
        }

        void Deactivate() override
        {
        }

        static void Reflect(AZ::ReflectContext* reflection)
        {
            if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection))
            {
                serializeContext->Class<ComponentWithEntityReference, AZ::Component>()
                    ->Field("EntityReference", &ComponentWithEntityReference::m_entityReference)
                    ;
            }
        }

        AZ::EntityId m_entityReference;
    };

    class SourceSpawnableComponent : public AZ::Component
    {
    public:
        AZ_COMPONENT(SourceSpawnableComponent, "{47FF79CE-A95B-420E-8BEB-F1CC58087B87}");

        void Activate() override {}
        void Deactivate() override {}

        static void Reflect(AZ::ReflectContext* reflection)
        {
            if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection))
            {
                serializeContext->Class<SourceSpawnableComponent, AZ::Component>();
            }
        }
    };

    class TargetSpawnableComponent : public AZ::Component
    {
    public:
        AZ_COMPONENT(TargetSpawnableComponent, "{B4041561-63A7-4E1E-80F1-78C08D497960}");

        TargetSpawnableComponent() = default;
        explicit TargetSpawnableComponent(AZ::EntityId parent)
            : m_parent(parent)
        {
        }

        void Activate() override {}
        void Deactivate() override {}

        static void Reflect(AZ::ReflectContext* reflection)
        {
            if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection))
            {
                serializeContext->Class<TargetSpawnableComponent, AZ::Component>()
                    ->Field("Parent", &TargetSpawnableComponent::m_parent);
            }
        }

        AZ::EntityId m_parent;
    };

    class SpawnableEntitiesManagerTest : public LeakDetectionFixture
    {
    public:
        constexpr static AZ::u64 EntityIdStartId = 40;

        void SetUp() override
        {
            LeakDetectionFixture::SetUp();

            m_application = new TestApplication();
            AZ::ComponentApplication::Descriptor descriptor;
            AZ::ComponentApplication::StartupParameters startupParameters;
            startupParameters.m_loadSettingsRegistry = false;
            m_application->Start(descriptor, startupParameters);
            m_application->RegisterComponentDescriptor(ComponentWithEntityReference::CreateDescriptor());
            m_application->RegisterComponentDescriptor(SourceSpawnableComponent::CreateDescriptor());
            m_application->RegisterComponentDescriptor(TargetSpawnableComponent::CreateDescriptor());

            // Without this, the user settings component would attempt to save on finalize/shutdown. Since the file is
            // shared across the whole engine, if multiple tests are run in parallel, the saving could cause a crash
            // in the unit tests.
            AZ::UserSettingsComponentRequestBus::Broadcast(&AZ::UserSettingsComponentRequests::DisableSaveOnFinalize);
            
            m_spawnable = aznew AzFramework::Spawnable(
                AZ::Data::AssetId::CreateString("{EB2E8A2B-F253-4A90-BBF4-55F2EED786B8}:0"), AZ::Data::AssetData::AssetStatus::Ready);
            m_spawnableAsset = new AZ::Data::Asset<AzFramework::Spawnable>(m_spawnable, AZ::Data::AssetLoadBehavior::Default);
            m_ticket = aznew AzFramework::EntitySpawnTicket(*m_spawnableAsset);

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

            LeakDetectionFixture::TearDown();
        }

        void ProcessQueueTillEmtpy()
        {
            for (size_t i=0; i<1000; ++i) // Don't do this indefinitely to avoid deadlocking on a failing test.
            {
                if (m_manager->ProcessQueue(
                        AzFramework::SpawnableEntitiesManager::CommandQueuePriority::High |
                        AzFramework::SpawnableEntitiesManager::CommandQueuePriority::Regular) ==
                    AzFramework::SpawnableEntitiesManager::CommandQueueStatus::NoCommandsLeft)
                {
                    break;
                }
            }
        }

        void FillSpawnable(size_t numElements)
        {
            AzFramework::Spawnable::EntityList& entities = m_spawnable->GetEntities();
            entities.clear();
            entities.reserve(numElements);
            for (size_t i=0; i<numElements; ++i)
            {
                auto entry = AZStd::make_unique<AZ::Entity>();
                entry->AddComponent(aznew SourceSpawnableComponent());
                entry->SetId(AZ::EntityId(EntityIdStartId + i));
                entities.push_back(AZStd::move(entry));
            }
        }

        AZ::Data::Asset<AzFramework::Spawnable> CreateTargetSpawnable(size_t numElements, bool requiresMatchingEntityIds)
        {
            auto target = aznew AzFramework::Spawnable(
                AZ::Data::AssetId(AZ::Uuid("{716CD8C3-0BA8-4F32-B579-0EC7C967796F}")), AZ::Data::AssetData::AssetStatus::Ready);

            AzFramework::Spawnable::EntityList& entities = target->GetEntities();
            entities.reserve(numElements);
            if (requiresMatchingEntityIds)
            {
                for (size_t i = 0; i < numElements; ++i)
                {
                    auto entry = AZStd::make_unique<AZ::Entity>();
                    if (i != 0)
                    {
                        entry->AddComponent(aznew TargetSpawnableComponent(AZ::EntityId(EntityIdStartId + i - 1)));
                    }
                    else
                    {
                        entry->AddComponent(aznew TargetSpawnableComponent());
                    }
                    entry->SetId(AZ::EntityId(EntityIdStartId + i));
                    entities.push_back(AZStd::move(entry));
                }
            }
            else
            {
                for (size_t i = 0; i < numElements; ++i)
                {
                    auto entry = AZStd::make_unique<AZ::Entity>();
                    entry->AddComponent(aznew TargetSpawnableComponent());
                    entities.push_back(AZStd::move(entry));
                }
            }

            return AZ::Data::Asset<AzFramework::Spawnable>(target, AZ::Data::AssetLoadBehavior::NoLoad);
        }

        template<size_t AliasCount>
        void InsertEntityAliases(
            const AZStd::array<uint32_t, AliasCount>& sourceIds,
            const AZStd::array<uint32_t, AliasCount>& targetIds,
            const AZStd::array<AzFramework::Spawnable::EntityAliasType, AliasCount>& aliasTypes,
            AZ::Data::Asset<AzFramework::Spawnable>* target = nullptr)
        {
            AzFramework::Spawnable::EntityAliasVisitor visitor = m_spawnable->TryGetAliases();

            for (uint32_t i = 0; i < AliasCount; ++i)
            {
                if (target)
                {
                    visitor.AddAlias(*target, AZ::Crc32(i), sourceIds[i], targetIds[i], aliasTypes[i], false);
                }
                else
                {
                    AZ::Data::Asset<AzFramework::Spawnable> spawnable(
                        AZ::Data::AssetId(AZ::Uuid("{4CBEC17A-52D6-42D5-9037-F4C05B9CE1D9}"), i), azrtti_typeid<AzFramework::Spawnable>());
                    visitor.AddAlias(AZStd::move(spawnable), AZ::Crc32(i), sourceIds[i], targetIds[i], aliasTypes[i], false);
                }
            }
        }

        static bool AreAllEntitiesReplaced(AzFramework::SpawnableConstEntityContainerView entities)
        {
            for (const AZ::Entity* entity : entities)
            {
                if (entity)
                {
                    if (entity->FindComponent<SourceSpawnableComponent>() != nullptr ||
                        entity->FindComponent<TargetSpawnableComponent>() == nullptr)
                    {
                        return false;
                    }
                }
                else
                {
                    return false;
                }
            }
            return true;
        }

        static bool DoParentEntityIdsMatch(AzFramework::SpawnableConstEntityContainerView entities)
        {
            if (entities.empty())
            {
                return false;
            }

            const AZ::Entity* previous = nullptr;
            for (const AZ::Entity* entity : entities)
            {
                if (entity)
                {
                    if (previous)
                    {
                        if (TargetSpawnableComponent* link = entity->FindComponent<TargetSpawnableComponent>(); link != nullptr)
                        {
                            if (link->m_parent != previous->GetId())
                            {
                                return false;
                            }
                        }
                        previous = entity;
                    }
                }
                else
                {
                    return false;
                }
            }
            return true;
        }

        static bool IsEveryOtherEntityAReplacement(AzFramework::SpawnableConstEntityContainerView entities)
        {
            bool onAlternative = true;
            for (const AZ::Entity* entity : entities)
            {
                if (entity)
                {
                    if (onAlternative)
                    {
                        if (entity->FindComponent<SourceSpawnableComponent>() == nullptr ||
                            entity->FindComponent<TargetSpawnableComponent>() != nullptr)
                        {
                            return false;
                        }
                    }
                    else
                    {
                        if (entity->FindComponent<SourceSpawnableComponent>() != nullptr ||
                            entity->FindComponent<TargetSpawnableComponent>() == nullptr)
                        {
                            return false;
                        }
                    }
                    onAlternative = !onAlternative;
                }
                else
                {
                    return false;
                }
            }
            return true;
        }

        static bool AreAllMerged(AzFramework::SpawnableConstEntityContainerView entities)
        {
            for (const AZ::Entity* entity : entities)
            {
                if (entity)
                {
                    if (entity->FindComponent<SourceSpawnableComponent>() == nullptr ||
                        entity->FindComponent<TargetSpawnableComponent>() == nullptr)
                    {
                        return false;
                    }
                }
                else
                {
                    return false;
                }
            }
            return true;
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

        enum class EntityReferenceScheme
        {
            AllReferenceFirst,
            AllReferenceLast,
            AllReferenceThemselves,
            AllReferenceNextCircular,
            AllReferencePreviousCircular
        };

        void CreateEntityReferences(EntityReferenceScheme refScheme)
        {
            AzFramework::Spawnable::EntityList& entities = m_spawnable->GetEntities();
            size_t numElements = entities.size();
            for (size_t i = 0; i < numElements; ++i)
            {
                AZStd::unique_ptr<AZ::Entity>& entity = entities[i];
                auto component = entity->CreateComponent<ComponentWithEntityReference>();
                switch (refScheme)
                {
                case EntityReferenceScheme::AllReferenceFirst :
                    component->m_entityReference = entities[0]->GetId();
                    break;
                case EntityReferenceScheme::AllReferenceLast:
                    component->m_entityReference = entities[numElements - 1]->GetId();
                    break;
                case EntityReferenceScheme::AllReferenceThemselves:
                    component->m_entityReference = entities[i]->GetId();
                    break;
                case EntityReferenceScheme::AllReferenceNextCircular:
                    component->m_entityReference = entities[(i + 1) % numElements]->GetId();
                    break;
                case EntityReferenceScheme::AllReferencePreviousCircular:
                    component->m_entityReference = entities[(i + numElements - 1) % numElements]->GetId();
                    break;
                }
            }
        }

        // Verify that the entity references are pointing to the correct other entities within the same spawn batch.
        // A "spawn batch" is the set of entities produced for each SpawnAllEntities command.
        void ValidateEntityReferences(
            EntityReferenceScheme refScheme, size_t entitiesPerBatch, AzFramework::SpawnableConstEntityContainerView entities)
        {
            size_t numElements = entities.size();

            for (size_t i = 0; i < numElements; ++i)
            {
                // Calculate the element offset that's the start of each batch of entities spawned.
                size_t curSpawnBatch = i / entitiesPerBatch;
                size_t curBatchOffset = curSpawnBatch * entitiesPerBatch;
                size_t curBatchIndex = i - curBatchOffset;

                const AZ::Entity* const entity = *(entities.begin() + i);

                auto component = entity->FindComponent<ComponentWithEntityReference>();
                ASSERT_NE(nullptr, component);
                AZ::EntityId comparisonId;
                // Ids should be local to a batch, so each of these will be compared within a batch of entities, not globally across
                // the entire set.
                switch (refScheme)
                {
                case EntityReferenceScheme::AllReferenceFirst:
                    // Compare against the first entity in each batch
                    comparisonId = (*(entities.begin() + curBatchOffset))->GetId();
                    break;
                case EntityReferenceScheme::AllReferenceLast:
                    // Compare against the last entity in each batch
                    comparisonId = (*(entities.begin() + curBatchOffset + (entitiesPerBatch - 1)))->GetId();
                    break;
                case EntityReferenceScheme::AllReferenceThemselves:
                    // Compare against itself
                    comparisonId = entity->GetId();
                    break;
                case EntityReferenceScheme::AllReferenceNextCircular:
                    // Compare against the next entity in each batch, looping around so that the last entity in the batch should refer
                    // to the first entity in the batch.
                    comparisonId = (*(entities.begin() + curBatchOffset + ((curBatchIndex + 1) % entitiesPerBatch)))->GetId();
                    break;
                case EntityReferenceScheme::AllReferencePreviousCircular:
                    // Compare against the previous entity in each batch, looping around so that the first entity in the batch should refer
                    // to the last entity in the batch.
                    comparisonId = (*(entities.begin() + curBatchOffset + ((curBatchIndex + numElements - 1) % entitiesPerBatch)))->GetId();
                    break;
                }
                EXPECT_EQ(comparisonId, component->m_entityReference);
            }
        };

    protected:
        AZ::Data::Asset<AzFramework::Spawnable>* m_spawnableAsset { nullptr };
        AzFramework::SpawnableEntitiesManager* m_manager { nullptr };
        AzFramework::EntitySpawnTicket* m_ticket { nullptr };
        AzFramework::Spawnable* m_spawnable { nullptr };
        TestApplication* m_application { nullptr };
    };


    //
    // Constructors
    //

    TEST_F(SpawnableEntitiesManagerTest, EntitySpawnTicket_Move_Works)
    {
        AzFramework::EntitySpawnTicket ticket1(*m_spawnableAsset);
        AzFramework::EntitySpawnTicket ticket2(*m_spawnableAsset);

        const AzFramework::EntitySpawnTicket::Id ticket1Id = ticket1.GetId();
        const AzFramework::EntitySpawnTicket::Id ticket2Id = ticket2.GetId();

        AzFramework::EntitySpawnTicket ticketMoveConstructor(AZStd::move(ticket1));
        EXPECT_TRUE(ticketMoveConstructor.IsValid());
        EXPECT_EQ(ticketMoveConstructor.GetId(), ticket1Id);

        AzFramework::EntitySpawnTicket ticketMoveOperator;
        ticketMoveOperator = AZStd::move(ticket2);
        EXPECT_TRUE(ticketMoveOperator.IsValid());
        EXPECT_EQ(ticketMoveOperator.GetId(), ticket2Id);
    }


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
        ProcessQueueTillEmtpy();

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
        ProcessQueueTillEmtpy();
    }

    TEST_F(SpawnableEntitiesManagerTest, SpawnAllEntities_AllEntitiesReferenceOtherEntities_EntityIdsAreMappedCorrectly)
    {
        // This tests that entity id references get mapped correctly in a SpawnAllEntities call whether they're forward referencing
        // in the list, backwards referencing, or self-referencing.  The circular tests are to ensure the implementation works regardless
        // of entity ordering.
        for (EntityReferenceScheme refScheme : {
                EntityReferenceScheme::AllReferenceFirst, EntityReferenceScheme::AllReferenceLast, 
                EntityReferenceScheme::AllReferenceThemselves, EntityReferenceScheme::AllReferenceNextCircular,
                EntityReferenceScheme::AllReferencePreviousCircular })
        {
            constexpr size_t NumEntities = 4;
            FillSpawnable(NumEntities);
            CreateEntityReferences(refScheme);

            auto callback = [this, refScheme]
                (AzFramework::EntitySpawnTicket::Id, AzFramework::SpawnableConstEntityContainerView entities)
            {
                ValidateEntityReferences(refScheme, NumEntities, entities);
            };
            AzFramework::SpawnAllEntitiesOptionalArgs optionalArgs;
            optionalArgs.m_completionCallback = AZStd::move(callback);
            m_manager->SpawnAllEntities(*m_ticket, AZStd::move(optionalArgs));
            ProcessQueueTillEmtpy();
        }
    }

    TEST_F(SpawnableEntitiesManagerTest, SpawnAllEntities_AllEntitiesReferenceOtherEntities_EntityIdsOnlyReferWithinASingleCall)
    {
        // This tests that entity id references get mapped correctly with multiple SpawnAllEntities calls.  Each call should only map
        // the entities to other entities within the same call, regardless of forward or backward mapping.
        // For example, suppose entities 1, 2, and 3 refer to 4.  In the first SpawnAllEntities call, entities 1-3 will refer to 4.
        // In the second SpawnAllEntities call, entities 1-3 will refer to the second 4, not the previously-spawned 4.
        for (EntityReferenceScheme refScheme :
            { EntityReferenceScheme::AllReferenceFirst, EntityReferenceScheme::AllReferenceLast,
               EntityReferenceScheme::AllReferenceThemselves, EntityReferenceScheme::AllReferenceNextCircular,
               EntityReferenceScheme::AllReferencePreviousCircular
            })
        {
            // Make sure we start with a fresh ticket each time, or else each iteration through this loop would continue to build up
            // more and more entities.
            delete m_ticket;
            m_ticket = aznew AzFramework::EntitySpawnTicket(*m_spawnableAsset);

            constexpr size_t NumEntities = 4;
            FillSpawnable(NumEntities);
            CreateEntityReferences(refScheme);

            auto callback = [this, refScheme]
                (AzFramework::EntitySpawnTicket::Id, AzFramework::SpawnableConstEntityContainerView entities)
            {
                ValidateEntityReferences(refScheme, NumEntities, entities);
            };

            // Spawn twice.
            constexpr size_t NumSpawnAllCalls = 2;
            for (int spawns = 0; spawns < NumSpawnAllCalls; spawns++)
            {
                AzFramework::SpawnAllEntitiesOptionalArgs optionalArgs;
                optionalArgs.m_completionCallback = AZStd::move(callback);
                m_manager->SpawnAllEntities(*m_ticket, AZStd::move(optionalArgs));
            }

            m_manager->ListEntities(*m_ticket, callback);
            ProcessQueueTillEmtpy();
        }
    }

    TEST_F(SpawnableEntitiesManagerTest, SpawnAllEntities_DeleteTicketBeforeCall_NoCrash)
    {
        {
            AzFramework::EntitySpawnTicket ticket(*m_spawnableAsset);
            m_manager->SpawnAllEntities(ticket);
        }
        ProcessQueueTillEmtpy();
    }

    TEST_F(SpawnableEntitiesManagerTest, SpawnAllEntities_AllAliasesWithDisabled_NoEntitiesSpawned)
    {
        using namespace AzFramework;
        static constexpr size_t NumEntities = 4;
        FillSpawnable(NumEntities);
        InsertEntityAliases<NumEntities>(
            { 0, 1, 2, 3 }, { 0, 1, 2, 3 },
            { Spawnable::EntityAliasType::Disable, Spawnable::EntityAliasType::Disable, Spawnable::EntityAliasType::Disable,
              Spawnable::EntityAliasType::Disable });

        size_t spawnedEntitiesCount = 0;
        auto callback = [&spawnedEntitiesCount](AzFramework::EntitySpawnTicket::Id, AzFramework::SpawnableConstEntityContainerView entities)
        {
            spawnedEntitiesCount += entities.size();
        };
        AzFramework::SpawnAllEntitiesOptionalArgs optionalArgs;
        optionalArgs.m_completionCallback = AZStd::move(callback);
        m_manager->SpawnAllEntities(*m_ticket, AZStd::move(optionalArgs));
        ProcessQueueTillEmtpy();

        EXPECT_EQ(0, spawnedEntitiesCount);
    }

    TEST_F(SpawnableEntitiesManagerTest, SpawnAllEntities_SomeAliasesWithDisabled_RegularEntitiesAreSpawned)
    {
        using namespace AzFramework;
        static constexpr size_t NumEntities = 8;
        FillSpawnable(NumEntities);
        InsertEntityAliases<2>({ 1, 3 }, { 1, 3 }, { Spawnable::EntityAliasType::Disable, Spawnable::EntityAliasType::Disable });

        size_t spawnedEntitiesCount = 0;
        auto callback = [&spawnedEntitiesCount](AzFramework::EntitySpawnTicket::Id, AzFramework::SpawnableConstEntityContainerView entities)
        {
            spawnedEntitiesCount += entities.size();
        };
        AzFramework::SpawnAllEntitiesOptionalArgs optionalArgs;
        optionalArgs.m_completionCallback = AZStd::move(callback);
        m_manager->SpawnAllEntities(*m_ticket, AZStd::move(optionalArgs));
        ProcessQueueTillEmtpy();

        EXPECT_EQ(6, spawnedEntitiesCount);
    }

    TEST_F(SpawnableEntitiesManagerTest, SpawnAllEntities_AllAliasesWithReplace_EntitiesSpawnedFromTarget)
    {
        using namespace AzFramework;
        static constexpr size_t NumEntities = 4;
        FillSpawnable(NumEntities);
        constexpr bool requiresMatchingEntityIds = true;
        AZ::Data::Asset<Spawnable> target = CreateTargetSpawnable(4, requiresMatchingEntityIds);
        InsertEntityAliases<4>(
            { 0, 1, 2, 3 }, { 0, 1, 2, 3 },
            { Spawnable::EntityAliasType::Replace, Spawnable::EntityAliasType::Replace, Spawnable::EntityAliasType::Replace,
              Spawnable::EntityAliasType::Replace },
            &target);

        size_t spawnedEntitiesCount = 0;
        bool allReplaced = false;
        bool allEntityIdsPatched = false;
        auto callback = [&spawnedEntitiesCount, &allReplaced, &allEntityIdsPatched](
            AzFramework::EntitySpawnTicket::Id, AzFramework::SpawnableConstEntityContainerView entities)
        {
            spawnedEntitiesCount += entities.size();
            allReplaced = AreAllEntitiesReplaced(entities);
            allEntityIdsPatched = DoParentEntityIdsMatch(entities);
        };
        AzFramework::SpawnAllEntitiesOptionalArgs optionalArgs;
        optionalArgs.m_completionCallback = AZStd::move(callback);
        m_manager->SpawnAllEntities(*m_ticket, AZStd::move(optionalArgs));
        ProcessQueueTillEmtpy();

        EXPECT_EQ(4, spawnedEntitiesCount);
        EXPECT_TRUE(allReplaced);
        EXPECT_TRUE(allEntityIdsPatched);
    }

    TEST_F(SpawnableEntitiesManagerTest, SpawnAllEntities_AllAliasesWithAdditional_SourceAndTargetComponentsMerged)
    {
        using namespace AzFramework;
        static constexpr size_t NumEntities = 4;
        FillSpawnable(NumEntities);
        constexpr bool requiresMatchingEntityIds = false;
        AZ::Data::Asset<Spawnable> target = CreateTargetSpawnable(4, requiresMatchingEntityIds);
        InsertEntityAliases<4>(
            { 0, 1, 2, 3 }, { 0, 1, 2, 3 },
            { Spawnable::EntityAliasType::Additional, Spawnable::EntityAliasType::Additional, Spawnable::EntityAliasType::Additional,
              Spawnable::EntityAliasType::Additional },
            &target);

        size_t spawnedEntitiesCount = 0;
        bool allAdded = false;
        bool allEntityIdsPatched = false;
        auto callback = [&spawnedEntitiesCount, &allAdded, &allEntityIdsPatched](
            AzFramework::EntitySpawnTicket::Id, AzFramework::SpawnableConstEntityContainerView entities)
        {
            spawnedEntitiesCount += entities.size();
            allAdded = IsEveryOtherEntityAReplacement(entities);
            allEntityIdsPatched = DoParentEntityIdsMatch(entities);
        };
        AzFramework::SpawnAllEntitiesOptionalArgs optionalArgs;
        optionalArgs.m_completionCallback = AZStd::move(callback);
        m_manager->SpawnAllEntities(*m_ticket, AZStd::move(optionalArgs));
        ProcessQueueTillEmtpy();

        EXPECT_EQ(8, spawnedEntitiesCount);
        EXPECT_TRUE(allAdded);
        EXPECT_TRUE(allEntityIdsPatched);
    }

    TEST_F(SpawnableEntitiesManagerTest, SpawnAllEntities_AllAliasesWithMerge_SourceAndTargetComponentsMerged)
    {
        using namespace AzFramework;
        static constexpr size_t NumEntities = 4;
        FillSpawnable(NumEntities);
        constexpr bool requiresMatchingEntityIds = true;
        AZ::Data::Asset<Spawnable> target = CreateTargetSpawnable(4, requiresMatchingEntityIds);
        InsertEntityAliases<4>(
            { 0, 1, 2, 3 }, { 0, 1, 2, 3 },
            { Spawnable::EntityAliasType::Merge, Spawnable::EntityAliasType::Merge, Spawnable::EntityAliasType::Merge,
              Spawnable::EntityAliasType::Merge },
            &target);

        size_t spawnedEntitiesCount = 0;
        bool allMerged = false;
        bool allEntityIdsPatched = false;
        auto callback = [&spawnedEntitiesCount, &allMerged, &allEntityIdsPatched](
            AzFramework::EntitySpawnTicket::Id, AzFramework::SpawnableConstEntityContainerView entities)
        {
            spawnedEntitiesCount += entities.size();
            allMerged = AreAllMerged(entities);
            allEntityIdsPatched = DoParentEntityIdsMatch(entities);
        };
        AzFramework::SpawnAllEntitiesOptionalArgs optionalArgs;
        optionalArgs.m_completionCallback = AZStd::move(callback);
        m_manager->SpawnAllEntities(*m_ticket, AZStd::move(optionalArgs));
        ProcessQueueTillEmtpy();

        EXPECT_EQ(4, spawnedEntitiesCount);
        EXPECT_TRUE(allMerged);
        EXPECT_TRUE(allEntityIdsPatched);
    }

    //
    // SpawnEntities
    //

    TEST_F(SpawnableEntitiesManagerTest, SpawnEntities_Call_AllEntitiesSpawned)
    {
        static constexpr size_t NumEntities = 4;
        FillSpawnable(NumEntities);

        AZStd::vector<uint32_t> indices = { 0, 2, 3, 1 };

        size_t spawnedEntitiesCount = 0;
        auto callback = [&spawnedEntitiesCount](AzFramework::EntitySpawnTicket::Id, AzFramework::SpawnableConstEntityContainerView entities)
        {
            spawnedEntitiesCount += entities.size();
        };
        AzFramework::SpawnEntitiesOptionalArgs optionalArgs;
        optionalArgs.m_completionCallback = AZStd::move(callback);
        m_manager->SpawnEntities(*m_ticket, AZStd::move(indices), AZStd::move(optionalArgs));
        ProcessQueueTillEmtpy();

        EXPECT_EQ(NumEntities, spawnedEntitiesCount);
    }

    TEST_F(SpawnableEntitiesManagerTest, SpawnEntities_SpawnTheSameEntity_AllEntitiesSpawned)
    {
        static constexpr size_t NumEntities = 1;
        FillSpawnable(NumEntities);

        AZStd::vector<uint32_t> indices = { 0, 0 };

        size_t spawnedEntitiesCount = 0;
        auto callback =
            [&spawnedEntitiesCount](AzFramework::EntitySpawnTicket::Id, AzFramework::SpawnableConstEntityContainerView entities)
        {
            spawnedEntitiesCount += entities.size();
        };
        AzFramework::SpawnEntitiesOptionalArgs optionalArgs;
        optionalArgs.m_completionCallback = AZStd::move(callback);
        m_manager->SpawnEntities(*m_ticket, AZStd::move(indices), AZStd::move(optionalArgs));
        ProcessQueueTillEmtpy();

        EXPECT_EQ(NumEntities * 2, spawnedEntitiesCount);
    }

    TEST_F(SpawnableEntitiesManagerTest, SpawnEntities_MultipleSpawns_AllEntitiesSpawned)
    {
        static constexpr size_t NumEntities = 4;
        FillSpawnable(NumEntities);

        AZStd::vector<uint32_t> indices = { 0, 2, 3, 1 };

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
        ProcessQueueTillEmtpy();

        EXPECT_EQ(NumEntities * 2, spawnedEntitiesCount);
    }

    TEST_F(SpawnableEntitiesManagerTest, SpawnEntities_ReferencesAreRemappedForNewBatch_AllPointToLatestParent)
    {
        static constexpr size_t NumEntities = 4;
        FillSpawnable(NumEntities);
        CreateSingleParent();

        AZStd::vector<uint32_t> indices = { 0, 1, 2, 3 };
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
        ProcessQueueTillEmtpy();

        EXPECT_NE(parents[0], parents[1]);
    }

    TEST_F(SpawnableEntitiesManagerTest, SpawnEntities_ReferencesAreRemappedForContinuedBatch_AllPointToLatestParent)
    {
        static constexpr size_t NumEntities = 4;
        FillSpawnable(NumEntities);
        CreateSingleParent();

        AZStd::vector<uint32_t> indices = { 0, 1, 2, 3 };
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
        ProcessQueueTillEmtpy();

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

        ProcessQueueTillEmtpy();
    }

    TEST_F(SpawnableEntitiesManagerTest, SpawnEntities_AllEntitiesReferenceOtherEntities_ForwardReferencesWorkInSingleCall)
    {
        constexpr EntityReferenceScheme refScheme = EntityReferenceScheme::AllReferenceNextCircular;
        constexpr size_t NumEntities = 4;
        FillSpawnable(NumEntities);
        CreateEntityReferences(refScheme);

        AZ_PUSH_DISABLE_WARNING(5233, "-Wunused-lambda-capture") // Older versions of MSVC toolchain require to pass constexpr in the
                                                                 // capture. Newer versions issue unused warning
        auto callback =
            [this, refScheme, NumEntities](AzFramework::EntitySpawnTicket::Id, AzFramework::SpawnableConstEntityContainerView entities)
        AZ_POP_DISABLE_WARNING
        {
            ValidateEntityReferences(refScheme, NumEntities, entities);
        };

        // Verify that by default, entities that refer to other entities that haven't been spawned yet have the correct references
        // when the spawning all occurs in the same call
        m_manager->SpawnEntities(*m_ticket, { 0, 1, 2, 3 });
        m_manager->ListEntities(*m_ticket, callback);
        ProcessQueueTillEmtpy();
    }

    TEST_F(SpawnableEntitiesManagerTest, SpawnEntities_AllEntitiesReferenceOtherEntities_ForwardReferencesWorkAcrossCalls)
    {
        constexpr EntityReferenceScheme refScheme = EntityReferenceScheme::AllReferenceNextCircular;
        constexpr size_t NumEntities = 4;
        FillSpawnable(NumEntities);
        CreateEntityReferences(refScheme);

        AZ_PUSH_DISABLE_WARNING(5233, "-Wunused-lambda-capture") // Older versions of MSVC toolchain require to pass constexpr in the
                                                                 // capture. Newer versions issue unused warning
        auto callback =
            [this, refScheme, NumEntities](AzFramework::EntitySpawnTicket::Id, AzFramework::SpawnableConstEntityContainerView entities)
        AZ_POP_DISABLE_WARNING
        {
            ValidateEntityReferences(refScheme, NumEntities, entities);
        };

        // Verify that by default, entities that refer to other entities that haven't been spawned yet have the correct references
        // even when the spawning is across multiple calls
        m_manager->SpawnEntities(*m_ticket, { 0 });
        m_manager->SpawnEntities(*m_ticket, { 1 });
        m_manager->SpawnEntities(*m_ticket, { 2 });
        m_manager->SpawnEntities(*m_ticket, { 3 });
        m_manager->ListEntities(*m_ticket, callback);
        ProcessQueueTillEmtpy();
    }

    TEST_F(SpawnableEntitiesManagerTest, SpawnEntities_AllEntitiesReferenceOtherEntities_ReferencesPointToFirstOrLatest)
    {
        // With SpawnEntities, entity references should either refer to the first entity that *will* be spawned, or the last entity
        // that *has* been spawned.  This test will create entities 0 1 2 3 that all refer to entity 3, and it will create two batches
        // of those.  In the first batch, they'll forward-reference.  In the second batch, they should backward-reference, except for
        // the second entity 3, which will now refer to itself as the last one that's been spawned.
        constexpr EntityReferenceScheme refScheme = EntityReferenceScheme::AllReferenceLast;
        constexpr size_t NumEntities = 4;
        FillSpawnable(NumEntities);
        CreateEntityReferences(refScheme);

        auto callback =
            [](AzFramework::EntitySpawnTicket::Id, AzFramework::SpawnableConstEntityContainerView entities)
        {
            size_t numElements = entities.size();

            for (size_t i = 0; i < numElements; ++i)
            {
                const AZ::Entity* const entity = *(entities.begin() + i);

                auto component = entity->FindComponent<ComponentWithEntityReference>();
                ASSERT_NE(nullptr, component);
                AZ::EntityId comparisonId;
                if (i < (numElements - 1))
                {
                    // There are two batches of NumEntities elements.  Every entity should either forward-reference or backward-reference
                    // to the last entity of the first batch, except for the very last entity of the second batch, which should reference
                    // itself.
                    comparisonId = (*(entities.begin() + (NumEntities- 1)))->GetId();
                }
                else
                {
                    // The very last entity of the second batch should reference itself because it's now the latest instance of that
                    // entity to be spawned.
                    comparisonId = entity->GetId();
                }

                EXPECT_EQ(comparisonId, component->m_entityReference);
            }
        };

        // Create 2 batches of forward references.  In the first batch, entities 0 1 2 will point forward to 3.  In the second batch,
        // entities 0 1 2 will point *backward* to the first 3, and the second entity 3 will point to itself.
        m_manager->SpawnEntities(*m_ticket, { 0, 1, 2, 3 });
        m_manager->SpawnEntities(*m_ticket, { 0, 1, 2, 3 });
        m_manager->ListEntities(*m_ticket, callback);
        ProcessQueueTillEmtpy();
    }

    TEST_F(SpawnableEntitiesManagerTest, SpawnEntities_AllEntitiesReferenceOtherEntities_MultipleSpawnsInSameCallReferenceCorrectly)
    {
        // With SpawnEntities, entity references should either refer to the first entity that *will* be spawned, or the last entity
        // that *has* been spawned.  This test will create entities 0 1 2 3 that all refer to entity 3, and it will create three sets
        // of those in the same call, with the following results:
        // - The first 0 1 2 will forward-reference to the first 3
        // - The first 3 will reference itself
        // - The second 0 1 2 will backwards-reference to the first 3
        // - The second 3 will reference itself
        // - The third 0 1 2 will backwards-reference to the second 3
        // - The third 3 will reference itself
        constexpr EntityReferenceScheme refScheme = EntityReferenceScheme::AllReferenceLast;
        constexpr size_t NumEntities = 4;
        FillSpawnable(NumEntities);
        CreateEntityReferences(refScheme);

        auto callback =
            [](AzFramework::EntitySpawnTicket::Id, AzFramework::SpawnableConstEntityContainerView entities)
        {
            size_t numElements = entities.size();

            for (size_t i = 0; i < numElements; ++i)
            {
                const AZ::Entity* const entity = *(entities.begin() + i);

                auto component = entity->FindComponent<ComponentWithEntityReference>();
                ASSERT_NE(nullptr, component);
                AZ::EntityId comparisonId;

                if (i < ((NumEntities * 2) - 1))
                {
                    // The first 7 entities (0 1 2 3 0 1 2) will all refer to the 4th one (1st '3').
                    comparisonId = (*(entities.begin() + (NumEntities - 1)))->GetId();
                }
                else if (i < (numElements - 1))
                {
                    // The next 4 entities (3 0 1 2) will all refer to the 8th one (2nd '3').
                    comparisonId = (*(entities.begin() + ((NumEntities * 2) - 1)))->GetId();
                }
                else
                {
                    // The very last entity (3) will reference itself (3rd '3').
                    comparisonId = entity->GetId();
                }

                EXPECT_EQ(comparisonId, component->m_entityReference);
            }
        };

        // Create the 3 batches of entities 0, 1, 2, 3.  The entity references should work as described at the top of the test.
        m_manager->SpawnEntities(*m_ticket, { 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3 });
        m_manager->ListEntities(*m_ticket, callback);
        ProcessQueueTillEmtpy();
    }

    TEST_F(SpawnableEntitiesManagerTest, SpawnEntities_AllEntitiesReferenceOtherEntities_OptionalFlagClearsReferenceMap)
    {
        constexpr EntityReferenceScheme refScheme = EntityReferenceScheme::AllReferenceLast;
        constexpr size_t NumEntities = 4;
        FillSpawnable(NumEntities);
        CreateEntityReferences(refScheme);

        AZ_PUSH_DISABLE_WARNING(5233, "-Wunused-lambda-capture") // Older versions of MSVC toolchain require to pass constexpr in the
                                                                 // capture. Newer versions issue unused warning
        auto callback =
            [this, refScheme, NumEntities](AzFramework::EntitySpawnTicket::Id, AzFramework::SpawnableConstEntityContainerView entities)
        AZ_POP_DISABLE_WARNING
        {
            ValidateEntityReferences(refScheme, NumEntities, entities);
        };

        // By setting the "referencePreviouslySpawnedEntities" flag to false, the map will get cleared on each call, so in both batches
        // the entities will forward-reference to the last entity in the batch.  If the flag were true, entities 0 1 2 in the second
        // batch would refer backwards to the first entity 3.

        AzFramework::SpawnEntitiesOptionalArgs optionalArgsSecondBatch;
        optionalArgsSecondBatch.m_completionCallback = AZStd::move(callback);
        optionalArgsSecondBatch.m_referencePreviouslySpawnedEntities = false;

        m_manager->SpawnEntities(*m_ticket, { 0, 1, 2, 3 }, optionalArgsSecondBatch);
        m_manager->SpawnEntities(*m_ticket, { 0, 1, 2, 3 }, AZStd::move(optionalArgsSecondBatch));
        m_manager->ListEntities(*m_ticket, callback);
        ProcessQueueTillEmtpy();
    }

    TEST_F(SpawnableEntitiesManagerTest, SpawnEntities_DeleteTicketBeforeCall_NoCrash)
    {
        {
            AzFramework::EntitySpawnTicket ticket(*m_spawnableAsset);
            m_manager->SpawnEntities(ticket, {/* Deliberate empty list of indices. */});
        }
        ProcessQueueTillEmtpy();
    }

    TEST_F(SpawnableEntitiesManagerTest, SpawnEntities_AllAliasesWithDisabled_NoEntitiesSpawned)
    {
        using namespace AzFramework;
        static constexpr size_t NumEntities = 4;
        FillSpawnable(NumEntities);

        InsertEntityAliases<NumEntities>(
            { 0, 1, 2, 3 }, { 0, 1, 2, 3 },
            { Spawnable::EntityAliasType::Disable, Spawnable::EntityAliasType::Disable, Spawnable::EntityAliasType::Disable,
              Spawnable::EntityAliasType::Disable });

        AZStd::vector<uint32_t> indices = { 0, 2, 3, 1 };

        size_t spawnedEntitiesCount = 0;
        auto callback = [&spawnedEntitiesCount](AzFramework::EntitySpawnTicket::Id, AzFramework::SpawnableConstEntityContainerView entities)
        {
            spawnedEntitiesCount += entities.size();
        };
        AzFramework::SpawnEntitiesOptionalArgs optionalArgs;
        optionalArgs.m_completionCallback = AZStd::move(callback);
        m_manager->SpawnEntities(*m_ticket, AZStd::move(indices), AZStd::move(optionalArgs));
        ProcessQueueTillEmtpy();

        EXPECT_EQ(0, spawnedEntitiesCount);
    }

    TEST_F(SpawnableEntitiesManagerTest, SpawnEntities_SomeAliasesWithDisabled_RegularEntitiesAreSpawned)
    {
        using namespace AzFramework;
        FillSpawnable(8);
        InsertEntityAliases<3>(
            { 1, 3, 6 }, { 1, 3, 6 },
            { Spawnable::EntityAliasType::Disable, Spawnable::EntityAliasType::Disable, Spawnable::EntityAliasType::Disable });

        AZStd::vector<uint32_t> indices = { 0, 2, 3, 1, 2, 3, 0, 1, 6, 4, 5, 7, 4, 1, 0, 6 };

        size_t spawnedEntitiesCount = 0;
        auto callback = [&spawnedEntitiesCount](AzFramework::EntitySpawnTicket::Id, AzFramework::SpawnableConstEntityContainerView entities)
        {
            spawnedEntitiesCount += entities.size();
        };
        AzFramework::SpawnEntitiesOptionalArgs optionalArgs;
        optionalArgs.m_completionCallback = AZStd::move(callback);
        m_manager->SpawnEntities(*m_ticket, AZStd::move(indices), AZStd::move(optionalArgs));
        ProcessQueueTillEmtpy();

        EXPECT_EQ(9, spawnedEntitiesCount);
    }

    TEST_F(SpawnableEntitiesManagerTest, SpawnEntities_AllAliasesWithReplace_EntitiesSpawnedFromTarget)
    {
        using namespace AzFramework;
        static constexpr size_t NumEntities = 4;
        FillSpawnable(NumEntities);
        constexpr bool requiresMatchingEntityIds = true;
        AZ::Data::Asset<Spawnable> target = CreateTargetSpawnable(4, requiresMatchingEntityIds);
        InsertEntityAliases<4>(
            { 0, 1, 2, 3 }, { 0, 1, 2, 3 },
            { Spawnable::EntityAliasType::Replace, Spawnable::EntityAliasType::Replace, Spawnable::EntityAliasType::Replace,
              Spawnable::EntityAliasType::Replace },
            &target);

        AZStd::vector<uint32_t> indices = { 0, 2, 3, 1 };

        size_t spawnedEntitiesCount = 0;
        bool allReplaced = false;
        bool allEntityIdsPatched = false;
        auto callback = [&spawnedEntitiesCount, &allReplaced, &allEntityIdsPatched](
            AzFramework::EntitySpawnTicket::Id, AzFramework::SpawnableConstEntityContainerView entities)
        {
            spawnedEntitiesCount += entities.size();
            allReplaced = AreAllEntitiesReplaced(entities);
            allEntityIdsPatched = DoParentEntityIdsMatch(entities);
        };
        AzFramework::SpawnEntitiesOptionalArgs optionalArgs;
        optionalArgs.m_completionCallback = AZStd::move(callback);
        m_manager->SpawnEntities(*m_ticket, AZStd::move(indices), AZStd::move(optionalArgs));
        ProcessQueueTillEmtpy();

        EXPECT_EQ(4, spawnedEntitiesCount);
        EXPECT_TRUE(allReplaced);
        EXPECT_TRUE(allEntityIdsPatched);
    }

    TEST_F(SpawnableEntitiesManagerTest, SpawnEntities_AllAliasesWithAdditional_SourceAndTargetComponentsMerged)
    {
        using namespace AzFramework;
        static constexpr size_t NumEntities = 4;
        FillSpawnable(NumEntities);
        constexpr bool requiresMatchingEntityIds = false;
        AZ::Data::Asset<Spawnable> target = CreateTargetSpawnable(4, requiresMatchingEntityIds);
        InsertEntityAliases<4>(
            { 0, 1, 2, 3 }, { 0, 1, 2, 3 },
            { Spawnable::EntityAliasType::Additional, Spawnable::EntityAliasType::Additional, Spawnable::EntityAliasType::Additional,
              Spawnable::EntityAliasType::Additional },
            &target);

        AZStd::vector<uint32_t> indices = { 0, 2, 3, 1 };

        size_t spawnedEntitiesCount = 0;
        bool allAdded = false;
        bool allEntityIdsPatched = false;
        auto callback =
            [&spawnedEntitiesCount, &allAdded, &allEntityIdsPatched](
            AzFramework::EntitySpawnTicket::Id, AzFramework::SpawnableConstEntityContainerView entities)
        {
            spawnedEntitiesCount += entities.size();
            allAdded = IsEveryOtherEntityAReplacement(entities);
            allEntityIdsPatched = DoParentEntityIdsMatch(entities);
        };
        AzFramework::SpawnEntitiesOptionalArgs optionalArgs;
        optionalArgs.m_completionCallback = AZStd::move(callback);
        m_manager->SpawnEntities(*m_ticket, AZStd::move(indices), AZStd::move(optionalArgs));
        ProcessQueueTillEmtpy();

        EXPECT_EQ(8, spawnedEntitiesCount);
        EXPECT_TRUE(allAdded);
        EXPECT_TRUE(allEntityIdsPatched);
    }

    TEST_F(SpawnableEntitiesManagerTest, SpawnEntities_AllAliasesWithMerge_SourceAndTargetComponentsMerged)
    {
        using namespace AzFramework;
        static constexpr size_t NumEntities = 4;
        FillSpawnable(NumEntities);
        constexpr bool requiresMatchingEntityIds = true;
        AZ::Data::Asset<Spawnable> target = CreateTargetSpawnable(4, requiresMatchingEntityIds);
        InsertEntityAliases<4>(
            { 0, 1, 2, 3 }, { 0, 1, 2, 3 },
            { Spawnable::EntityAliasType::Merge, Spawnable::EntityAliasType::Merge, Spawnable::EntityAliasType::Merge,
              Spawnable::EntityAliasType::Merge },
            &target);

        AZStd::vector<uint32_t> indices = { 0, 2, 3, 1 };

        size_t spawnedEntitiesCount = 0;
        bool allMerged = false;
        bool allEntityIdsPatched = false;
        auto callback = [&spawnedEntitiesCount, &allMerged, &allEntityIdsPatched](
            AzFramework::EntitySpawnTicket::Id, AzFramework::SpawnableConstEntityContainerView entities)
        {
            spawnedEntitiesCount += entities.size();
            allMerged = AreAllMerged(entities);
            allEntityIdsPatched = DoParentEntityIdsMatch(entities);
        };
        AzFramework::SpawnEntitiesOptionalArgs optionalArgs;
        optionalArgs.m_completionCallback = AZStd::move(callback);
        m_manager->SpawnEntities(*m_ticket, AZStd::move(indices), AZStd::move(optionalArgs));
        ProcessQueueTillEmtpy();

        EXPECT_EQ(4, spawnedEntitiesCount);
        EXPECT_TRUE(allMerged);
        EXPECT_TRUE(allEntityIdsPatched);
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
        ProcessQueueTillEmtpy();
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
        ProcessQueueTillEmtpy();
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
        ProcessQueueTillEmtpy();

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
        ProcessQueueTillEmtpy();
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
        ProcessQueueTillEmtpy();

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
        ProcessQueueTillEmtpy();
    }


    //
    // ClaimEntities
    //

    TEST_F(SpawnableEntitiesManagerTest, ClaimEntities_Call_AllEntitiesWereClaimedAndNotDeleted)
    {
        static constexpr size_t NumEntities = 4;
        FillSpawnable(NumEntities);

        AZStd::vector<AZ::Entity*> claimedEntities;
        auto callback = [&claimedEntities](AzFramework::EntitySpawnTicket::Id, AzFramework::SpawnableEntityContainerView container)
        {
            for (AZ::Entity* entity : container)
            {
                claimedEntities.push_back(entity);
            }
        };

        {
            AzFramework::EntitySpawnTicket ticket(*m_spawnableAsset);
            m_manager->SpawnAllEntities(ticket);
            m_manager->ClaimEntities(ticket, AZStd::move(callback));
            ProcessQueueTillEmtpy();
        }

        EXPECT_EQ(NumEntities, claimedEntities.size());

        // If these calls fail it means that the ticket has still deleted the entities, so they weren't properly claimed.
        for (AZ::Entity* entity : claimedEntities)
        {
            delete entity;
        }
    }

    TEST_F(SpawnableEntitiesManagerTest, ClaimEntities_DeleteTicketBeforeCall_NoCrash)
    {
        auto callback = [](AzFramework::EntitySpawnTicket::Id, AzFramework::SpawnableEntityContainerView) {};

        {
            AzFramework::EntitySpawnTicket ticket(*m_spawnableAsset);
            m_manager->ClaimEntities(ticket, AZStd::move(callback));
        }
        ProcessQueueTillEmtpy();
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
        ProcessQueueTillEmtpy();
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

        ProcessQueueTillEmtpy();

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
