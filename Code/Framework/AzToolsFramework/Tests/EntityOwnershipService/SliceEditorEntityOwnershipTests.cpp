/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/Entity/SliceEditorEntityOwnershipService.h>
#include "EntityOwnershipServiceTestFixture.h"

namespace UnitTest
{
    class SliceEditorEntityOwnershipTests
        : public EntityOwnershipServiceTestFixture
    {
    public:
        void SetUp() override
        {
            SetUpEntityOwnershipServiceTest();
            m_sliceEditorEntityOwnershipService = AZStd::make_unique<AzToolsFramework::SliceEditorEntityOwnershipService>(
                AZ::Uuid::CreateNull(), m_app->GetSerializeContext());

            m_sliceEditorEntityOwnershipService->SetEntitiesAddedCallback([this](const AzFramework::EntityList& entityList)
            {
                this->HandleEntitiesAdded(entityList);
            });

            m_sliceEditorEntityOwnershipService->SetEntitiesRemovedCallback([this](const AzFramework::EntityIdList& entityIds)
            {
                this->HandleEntitiesRemoved(entityIds);
            });

            m_sliceEditorEntityOwnershipService->SetValidateEntitiesCallback([this](const AzFramework::EntityList& entityList)
            {
                return this->ValidateEntities(entityList);
            });

            m_sliceEditorEntityOwnershipService->Initialize();
        }
        void TearDown() override
        {
            m_sliceEditorEntityOwnershipService->Destroy();
            m_sliceEditorEntityOwnershipService.reset();
            TearDownEntityOwnershipServiceTest();
        }
    protected:

        AZStd::unique_ptr<AzToolsFramework::SliceEditorEntityOwnershipService> m_sliceEditorEntityOwnershipService;
    };

    TEST_F(SliceEditorEntityOwnershipTests, Initialize_ResetOwnershipService_CreateRootSlice)
    {
        m_sliceEditorEntityOwnershipService->Reset();
        EXPECT_TRUE(GetRootSliceAsset()->GetComponent() != nullptr);
    }

    TEST_F(SliceEditorEntityOwnershipTests, OnAssetReloaded_RootAssetReloaded_ReloadEntities)
    {
        // Clone the root slice asset
        AZ::Data::Asset<AZ::SliceAsset> rootSliceAssetClone(GetRootSliceAsset().Get()->Clone(), AZ::Data::AssetLoadBehavior::Default);

        AZ::Entity* sliceRootEntity = new AZ::Entity();
        AZ::SliceComponent* sliceComponent = sliceRootEntity->CreateComponent<AZ::SliceComponent>();
        sliceComponent->SetSerializeContext(m_app->GetSerializeContext());
        sliceComponent->AddEntity(aznew AZ::Entity("testEntity"));
        rootSliceAssetClone->SetData(sliceRootEntity, sliceComponent);

        m_sliceEditorEntityOwnershipService->OnAssetReloaded(rootSliceAssetClone);

        // Validate that entities-added callback is triggerted.
        EXPECT_TRUE(m_entitiesAddedCallbackTriggered);

        const AzFramework::EntityList& entitiesUnderRootSlice = GetRootSliceAsset()->GetComponent()->GetNewEntities();

        // Validate that there is only one entity under root slice.
        EXPECT_EQ(entitiesUnderRootSlice.size(), 1);

        EXPECT_EQ(entitiesUnderRootSlice.at(0)->GetName(), "testEntity");
    }

    TEST_F(SliceEditorEntityOwnershipTests, LoadFromStream_RemapIdsFalse_IdsNotRemapped)
    {
        AZ::Entity* rootEntity = aznew AZ::Entity();
        AZ::SliceComponent* rootSliceComponent = rootEntity->CreateComponent<AZ::SliceComponent>();
        AZ::Entity* testEntity = aznew AZ::Entity();
        rootSliceComponent->AddEntity(testEntity);

        AZStd::vector<char> charBuffer;
        AZ::IO::ByteContainerStream<AZStd::vector<char>> stream(&charBuffer);
        AZ::Utils::SaveObjectToStream<AZ::Entity>(stream, AZ::ObjectStream::ST_XML, rootEntity, m_app->GetSerializeContext());
        stream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);

        EXPECT_TRUE(m_sliceEditorEntityOwnershipService->LoadFromStream(stream, false));
        
        AZ::SliceComponent::EntityIdToEntityIdMap previousToNewIdMap;
        AzFramework::SliceEntityOwnershipServiceRequestBus::BroadcastResult(previousToNewIdMap,
            &AzFramework::SliceEntityOwnershipServiceRequestBus::Events::GetLoadedEntityIdMap);

        // Verify that remapping of entityIds is not done by comparing the entityIds in previousToNewIdMap
        EXPECT_TRUE(previousToNewIdMap.begin()->first == previousToNewIdMap.begin()->second);

        delete rootEntity;
    }

    TEST_F(SliceEditorEntityOwnershipTests, InstantiateEditorSlice_ValidAssetProvided_SliceCreated)
    {
        AZ::Data::Asset<AZ::SliceAsset> sliceAsset;
        sliceAsset.Create(AZ::Data::AssetId(AZ::Uuid::CreateRandom()), false);

        AddEditorSlice(sliceAsset, AZ::Transform::CreateIdentity(), EntityList{});

        AZ::SliceComponent::SliceList& slicesUnderRootSlice = GetRootSliceAsset()->GetComponent()->GetSlices();
        ASSERT_EQ(slicesUnderRootSlice.size(), 1);

        // Verify that the created slice has the same asset as the one it's provided to be created with.
        EXPECT_EQ(sliceAsset, slicesUnderRootSlice.front().GetSliceAsset());
    }

    TEST_F(SliceEditorEntityOwnershipTests, PromoteEditorEntitiesIntoSlice_ValidEntitiesProvided_SliceCreated)
    {
        AZ::Entity* looseEntity = aznew AZ::Entity("testEntity");
        m_sliceEditorEntityOwnershipService->AddEntity(looseEntity);

        AZ::Entity* entityInSlice = aznew AZ::Entity("testEntity");
        AZ::Data::Asset<AZ::SliceAsset> sliceAsset;
        sliceAsset.Create(AZ::Data::AssetId(AZ::Uuid::CreateRandom()), false);
        AddSliceComponentToAsset(sliceAsset, EntityList{ entityInSlice });

        AZ::SliceComponent::EntityIdToEntityIdMap looseEntityIdToSliceAssetEntityIdMap;
        looseEntityIdToSliceAssetEntityIdMap.emplace(looseEntity->GetId(), entityInSlice->GetId());

        // Verify that no slices exist.
        AZ::SliceComponent::SliceList& slicesUnderRootSlice = GetRootSliceAsset()->GetComponent()->GetSlices();
        ASSERT_EQ(slicesUnderRootSlice.size(), 0);

        AzToolsFramework::SliceEditorEntityOwnershipServiceRequestBus::Broadcast(
            &AzToolsFramework::SliceEditorEntityOwnershipServiceRequestBus::Events::PromoteEditorEntitiesIntoSlice,
            sliceAsset, looseEntityIdToSliceAssetEntityIdMap);
        // Verify that one slice is created.
        slicesUnderRootSlice = GetRootSliceAsset()->GetComponent()->GetSlices();
        ASSERT_EQ(slicesUnderRootSlice.size(), 1);

        AZ::SliceComponent::SliceReference& sliceReference = slicesUnderRootSlice.front();

        // Verify that there exists one slice instance with one entity and the correct slice asset.
        ASSERT_EQ(sliceAsset, sliceReference.GetSliceAsset());
        ASSERT_EQ(sliceReference.GetInstances().size(), 1);
        AzFramework::EntityList entitiesOfSlice = sliceReference.GetInstances().begin()->GetInstantiated()->m_entities;
        ASSERT_EQ(entitiesOfSlice.size(), 1);

        // Verify that the entity in the created slice has the same id of the provided test entity.
        EXPECT_EQ(entitiesOfSlice[0]->GetId(), looseEntity->GetId());
    }

    TEST_F(SliceEditorEntityOwnershipTests, DetachSliceEntities_ValidEntitiesProvided_EntitiesDetached)
    {
        AZ::Data::Asset<AZ::SliceAsset> sliceAsset;
        sliceAsset.Create(AZ::Data::AssetId(AZ::Uuid::CreateRandom()), false);
        AZ::Entity* entityInSlice = aznew AZ::Entity("testEntity");
        AddEditorSlice(sliceAsset, AZ::Transform::CreateIdentity(), EntityList{ entityInSlice });

        // Verify that one slice is created and it has one editor entity
        AZ::SliceComponent::SliceList& slicesUnderRootSlice = GetRootSliceAsset()->GetComponent()->GetSlices();
        ASSERT_EQ(slicesUnderRootSlice.size(), 1);
        AZ::SliceComponent::SliceReference& sliceReference = slicesUnderRootSlice.front();
        AzFramework::EntityList entitiesOfSlice = sliceReference.GetInstances().begin()->GetInstantiated()->m_entities;
        ASSERT_EQ(entitiesOfSlice.size(), 1);

        // Verify that owning slice for the editor entity exists.
        AZ::SliceComponent::SliceInstanceAddress sliceInstanceAddressBeforeDetach;
        AzFramework::SliceEntityRequestBus::EventResult(sliceInstanceAddressBeforeDetach, entitiesOfSlice[0]->GetId(),
            &AzFramework::SliceEntityRequestBus::Events::GetOwningSlice);
        EXPECT_TRUE(sliceInstanceAddressBeforeDetach.IsValid());

        AzToolsFramework::SliceEditorEntityOwnershipServiceRequestBus::Broadcast(
            &AzToolsFramework::SliceEditorEntityOwnershipServiceRequestBus::Events::DetachSliceEntities,
            AzToolsFramework::EntityIdList{ entitiesOfSlice[0]->GetId() });

        // Verify that owning slice for the editor entity doesn't exist.
        AZ::SliceComponent::SliceInstanceAddress sliceInstanceAddressAfterDetach;
        AzFramework::SliceEntityRequestBus::EventResult(sliceInstanceAddressAfterDetach, entitiesOfSlice[0]->GetId(),
            &AzFramework::SliceEntityRequestBus::Events::GetOwningSlice);
        EXPECT_FALSE(sliceInstanceAddressAfterDetach.IsValid());
    }

    TEST_F(SliceEditorEntityOwnershipTests, DetachSliceInstances_ValidInstanceProvided_InstanceDetached)
    {
        AZ::Data::Asset<AZ::SliceAsset> sliceAsset;
        sliceAsset.Create(AZ::Data::AssetId(AZ::Uuid::CreateRandom()), false);
        AZ::Entity* testEntity = aznew AZ::Entity("testEntity");
        AddEditorSlice(sliceAsset, AZ::Transform::CreateIdentity(), EntityList{ testEntity });

        // Verify that one slice exists before detaching it.
        AZ::SliceComponent::SliceList& slicesUnderRootSlice = GetRootSliceAsset()->GetComponent()->GetSlices();
        ASSERT_EQ(slicesUnderRootSlice.front().GetInstances().size(), 1);

        // Verify that there are no loose entities in the editor.
        EntityList looseEntitiesBeforeDetach;
        m_sliceEditorEntityOwnershipService->GetNonPrefabEntities(looseEntitiesBeforeDetach);
        EXPECT_TRUE(looseEntitiesBeforeDetach.size() == 0);

        AZ::SliceComponent::SliceReference& sliceReference = slicesUnderRootSlice.front();
        auto sliceInstanceIterator = sliceReference.GetInstances().begin();
        AZ::SliceComponent::SliceInstanceAddress sliceInstanceAddress(&sliceReference, &(*sliceInstanceIterator));

        // Verify that there is one entity in the slice that is about to be detached.
        EntityList entitiesInsliceBeforeDetach = sliceInstanceIterator->GetInstantiated()->m_entities;
        EXPECT_TRUE(entitiesInsliceBeforeDetach.size() == 1);

        AzToolsFramework::SliceEditorEntityOwnershipServiceRequestBus::Broadcast(
            &AzToolsFramework::SliceEditorEntityOwnershipServiceRequestBus::Events::DetachSliceInstances,
            AZ::SliceComponent::SliceInstanceAddressSet{ sliceInstanceAddress });

        // Verify that the only slice that existed is not there anymore after detaching it.
        slicesUnderRootSlice = GetRootSliceAsset()->GetComponent()->GetSlices();
        EXPECT_EQ(slicesUnderRootSlice.front().GetInstances().size(), 0);

        // Verify that that the detached slice entity is now a loose entity in the editor.
        EntityList looseEntitiesAfterDetach;
        m_sliceEditorEntityOwnershipService->GetNonPrefabEntities(looseEntitiesAfterDetach);
        EXPECT_TRUE(looseEntitiesAfterDetach.size() == 1);
        EXPECT_EQ(entitiesInsliceBeforeDetach[0]->GetId(), looseEntitiesAfterDetach[0]->GetId());
    }

    TEST_F(SliceEditorEntityOwnershipTests, RestoreSliceEntity_SliceEntityDeleted_SliceEntityRestored)
    {
        AZ::Data::Asset<AZ::SliceAsset> sliceAsset;
        sliceAsset.Create(AZ::Data::AssetId(AZ::Uuid::CreateRandom()), false);
        AZ::Entity* testEntity = aznew AZ::Entity("testEntity");
        AddEditorSlice(sliceAsset, AZ::Transform::CreateIdentity(), EntityList{ testEntity });

        // Verify that one slice exists
        AZ::SliceComponent::SliceList& slicesUnderRootSlice = GetRootSliceAsset()->GetComponent()->GetSlices();
        ASSERT_EQ(slicesUnderRootSlice.front().GetInstances().size(), 1);

        AZ::SliceComponent::SliceReference& sliceReference = slicesUnderRootSlice.front();

        // Verify that one entity exists in the slice
        EntityList entitiesOfSlice = sliceReference.GetInstances().begin()->GetInstantiated()->m_entities;
        ASSERT_EQ(entitiesOfSlice.size(), 1);

        // Get the slice entity ancestor and slice instance id before destroying the entity of the slice
        AZ::SliceComponent::EntityAncestorList entityAncestorList;
        sliceReference.GetInstanceEntityAncestry(entitiesOfSlice.front()->GetId(), entityAncestorList);
        AZ::SliceComponent::SliceInstanceId sliceInstanceId = sliceReference.GetInstances().begin()->GetId();

        m_sliceEditorEntityOwnershipService->DestroyEntityById(entitiesOfSlice.front()->GetId());

        // Verify that no slices exists after slice entity is destroyed.
        ASSERT_EQ(slicesUnderRootSlice.size(), 0);

        // Restore the slice entity
        AZ::SliceComponent::EntityRestoreInfo entityRestoreInfo = AZ::SliceComponent::EntityRestoreInfo(
            sliceAsset, sliceInstanceId, entityAncestorList.front().m_entity->GetId(), AZ::DataPatch::FlagsMap{});
        AzToolsFramework::SliceEditorEntityOwnershipServiceRequestBus::Broadcast(
            &AzToolsFramework::SliceEditorEntityOwnershipServiceRequestBus::Events::RestoreSliceEntity,
            entitiesOfSlice.front(),
            entityRestoreInfo,
            AzToolsFramework::SliceEntityRestoreType::Deleted);
        AZ::TickBus::ExecuteQueuedEvents();

        // Verify that slice is restored with the same entity it had before.
        ASSERT_EQ(slicesUnderRootSlice.size(), 1);
        EntityList entitiesOfSliceAfterRestore = sliceReference.GetInstances().begin()->GetInstantiated()->m_entities;
        ASSERT_EQ(entitiesOfSliceAfterRestore.size(), 1);
        EXPECT_EQ(entitiesOfSliceAfterRestore.front()->GetId(), entitiesOfSlice.front()->GetId());
    }
}
