/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Entity/SliceEntityOwnershipService.h>
#include "EntityOwnershipServiceTestFixture.h"
#include <AzToolsFramework/Slice/SliceMetadataEntityContextBus.h>

namespace UnitTest
{
    class SliceEntityOwnershipTests
        : public EntityOwnershipServiceTestFixture
    {
    public:
        void SetUp() override
        {
            SetUpEntityOwnershipServiceTest();
            m_sliceEntityOwnershipService = AZStd::make_unique<AzFramework::SliceEntityOwnershipService>(AZ::Uuid::CreateNull(),
                m_app->GetSerializeContext());
            m_sliceEntityOwnershipService->Initialize();
            m_sliceEntityOwnershipService->SetEntitiesAddedCallback([this](const AzFramework::EntityList& entityList)
            {
                this->HandleEntitiesAdded(entityList);
            });

            m_sliceEntityOwnershipService->SetEntitiesRemovedCallback([this](const AzFramework::EntityIdList& entityIds)
            {
                this->HandleEntitiesRemoved(entityIds);
            });

            m_sliceEntityOwnershipService->SetValidateEntitiesCallback([this](const AzFramework::EntityList& entityList)
            {
                return this->ValidateEntities(entityList);
            });
        }
        void TearDown() override
        {
            m_sliceEntityOwnershipService->SetEntitiesAddedCallback(nullptr);

            // In order for the death tests to work, we have to destroy the EOS early. So, don't try to destroy again.
            if (m_sliceEntityOwnershipService->IsInitialized())
            {
                m_sliceEntityOwnershipService->Destroy();
            }

            m_sliceEntityOwnershipService.reset();

            TearDownEntityOwnershipServiceTest();
        }
    protected:
        AZStd::unique_ptr<AzFramework::SliceEntityOwnershipService> m_sliceEntityOwnershipService;
    };

    TEST_F(SliceEntityOwnershipTests, AddEntity_InitalizedCorrectly_EntityCreated)
    {
        AZ::Entity* testEntity = aznew AZ::Entity("testEntity");
        m_sliceEntityOwnershipService->AddEntity(testEntity);

        // Validate that entities-added callback is triggerted.
        EXPECT_TRUE(m_entitiesAddedCallbackTriggered);

        const AzFramework::EntityList& entitiesUnderRootSlice = GetRootSliceAsset()->GetComponent()->GetNewEntities();

        // Validate that there is only one entity under root slice.
        EXPECT_EQ(entitiesUnderRootSlice.size(), 1);

        EXPECT_EQ(entitiesUnderRootSlice.at(0)->GetName(), "testEntity");
    }

    TEST_F(SliceEntityOwnershipTests, DestroyEntityById_EntityAdded_EntityDestroyed)
    {
        AZ::Entity* testEntity = aznew AZ::Entity("testEntity");
        m_sliceEntityOwnershipService->AddEntity(testEntity);

        AzFramework::EntityList entitiesUnderRootSlice = GetRootSliceAsset()->GetComponent()->GetNewEntities();

        // Verify that entity is added
        EXPECT_EQ(entitiesUnderRootSlice.size(), 1);

        EXPECT_TRUE(m_sliceEntityOwnershipService->DestroyEntityById(testEntity->GetId()));

        entitiesUnderRootSlice = GetRootSliceAsset()->GetComponent()->GetNewEntities();

        // Verify that entity is destroyed
        EXPECT_EQ(entitiesUnderRootSlice.size(), 0);
    }

    TEST_F(SliceEntityOwnershipTests, GetRootSlice_RootAssetAbsent_ReturnNull)
    {
        m_sliceEntityOwnershipService->Destroy();
        AZ::SliceComponent* rootSlice = nullptr;
        AzFramework::SliceEntityOwnershipServiceRequestBus::BroadcastResult(rootSlice,
            &AzFramework::SliceEntityOwnershipServiceRequestBus::Events::GetRootSlice);
        EXPECT_EQ(rootSlice, nullptr);
    }

    TEST_F(SliceEntityOwnershipTests, GetRootSlice_RootAssetPresent_ReturnRootSlice)
    {
        AZ::SliceComponent* rootSlice = nullptr;
        AzFramework::SliceEntityOwnershipServiceRequestBus::BroadcastResult(rootSlice,
            &AzFramework::SliceEntityOwnershipServiceRequestBus::Events::GetRootSlice);
        EXPECT_NE(rootSlice, nullptr);
    }

    TEST_F(SliceEntityOwnershipTests, Reset_SliceAdded_DestroySliceEntities)
    {
        AzFramework::EntityList entitiesToAdd = AzFramework::EntityList{ aznew AZ::Entity() };
        AddSlice(entitiesToAdd);

        size_t slicesCountBeforeReset = GetRootSliceAsset()->GetComponent()->GetSlices().size();

        // Verify that slice exists
        EXPECT_EQ(slicesCountBeforeReset, 1);

        m_sliceEntityOwnershipService->Reset();

        size_t slicesCountAfterReset = GetRootSliceAsset()->GetComponent()->GetSlices().size();

        // Verify that slices under rootSlice were removed after reset of EntityOwnershipService.
        EXPECT_EQ(slicesCountAfterReset, 0);

        // Verify that call to destroy entities in the added slice occured.
        EXPECT_TRUE(m_entityRemovedCallbackTriggered);
    }

    TEST_F(SliceEntityOwnershipTests, Reset_SliceInstantiationStarted_StopSliceInstantiation)
    {
        AddSlice(AzFramework::EntityList{}, true);
        m_sliceEntityOwnershipService->Reset();
        AZ::TickBus::ExecuteQueuedEvents();

        size_t slicesCountUnderRootSlice = GetRootSliceAsset()->GetComponent()->GetSlices().size();
        EXPECT_EQ(slicesCountUnderRootSlice, 0);
    }

    TEST_F(SliceEntityOwnershipTests, Reset_EntityAdded_EntityDestroyedAfterReset)
    {
        AZ::Entity* testEntity = aznew AZ::Entity("testEntity");
        m_sliceEntityOwnershipService->AddEntity(testEntity);
        
        m_sliceEntityOwnershipService->Reset();

        const AzFramework::EntityList& entitiesUnderRootSlice = GetRootSliceAsset()->GetComponent()->GetNewEntities();

        EXPECT_EQ(entitiesUnderRootSlice.size(), 0);
        EXPECT_TRUE(m_entityRemovedCallbackTriggered);
    }

    TEST_F(SliceEntityOwnershipTests, HandleRootEntityReloadedFromStream_NoRootEntity_FailToLoadEntity)
    {
        bool rootEntityLoadSuccessful = false;
        AzFramework::SliceEntityOwnershipServiceRequestBus::BroadcastResult(rootEntityLoadSuccessful,
            &AzFramework::SliceEntityOwnershipServiceRequestBus::Events::HandleRootEntityReloadedFromStream, nullptr, false, nullptr);
        EXPECT_FALSE(rootEntityLoadSuccessful);
    }

    TEST_F(SliceEntityOwnershipTests, HandleRootEntityReloadedFromStream_NoSliceComponent_FailToLoadEntity)
    {
        AZ::Entity* testEntity = aznew AZ::Entity();

        // Suppress the AZ_Error thrown for not creating the root slice.
        AZ_TEST_START_TRACE_SUPPRESSION;
        bool rootEntityLoadSuccessful = false;
        AzFramework::SliceEntityOwnershipServiceRequestBus::BroadcastResult(rootEntityLoadSuccessful,
            &AzFramework::SliceEntityOwnershipServiceRequestBus::Events::HandleRootEntityReloadedFromStream, testEntity, false, nullptr);
        EXPECT_FALSE(rootEntityLoadSuccessful);
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);
        delete testEntity;
    }

    TEST_F(SliceEntityOwnershipTests, HandleRootEntityReloadedFromStream_RemapIdsTrue_IdsRemapped)
    {
        AZ::Entity* rootEntity = aznew AZ::Entity();
        AZ::SliceComponent* rootSliceComponent = rootEntity->CreateComponent<AZ::SliceComponent>();
        AZ::Entity* testEntity = aznew AZ::Entity();
        rootSliceComponent->AddEntity(testEntity);

        AZ::SliceComponent::EntityIdToEntityIdMap previousToNewIdMap;
        previousToNewIdMap.emplace(testEntity->GetId(), testEntity->GetId());

        bool rootEntityLoadSuccessful = false;
        AzFramework::SliceEntityOwnershipServiceRequestBus::BroadcastResult(rootEntityLoadSuccessful,
            &AzFramework::SliceEntityOwnershipServiceRequestBus::Events::HandleRootEntityReloadedFromStream,
            rootEntity, true, &previousToNewIdMap);

        EXPECT_TRUE(rootEntityLoadSuccessful);

        // Verify that remapping of entityIds is done by comparing the entityIds in previousToNewIdMap
        EXPECT_TRUE(previousToNewIdMap.begin()->first != previousToNewIdMap.begin()->second);
    }

    TEST_F(SliceEntityOwnershipTests, FindLoadedEntityIdMapping_IdsNotRemapped_EntityIdPresent)
    {
        AZ::Entity* rootEntity = aznew AZ::Entity();
        AZ::SliceComponent* rootSliceComponent = rootEntity->CreateComponent<AZ::SliceComponent>();
        AZ::Entity* testEntity = aznew AZ::Entity();
        rootSliceComponent->AddEntity(testEntity);

        bool rootEntityLoadSuccessful = false;
        AzFramework::SliceEntityOwnershipServiceRequestBus::BroadcastResult(rootEntityLoadSuccessful,
            &AzFramework::SliceEntityOwnershipServiceRequestBus::Events::HandleRootEntityReloadedFromStream, rootEntity, false, nullptr);
        EXPECT_TRUE(rootEntityLoadSuccessful);

        AZ::EntityId loadedEntityId;
        AzFramework::SliceEntityOwnershipServiceRequestBus::BroadcastResult(loadedEntityId,
            &AzFramework::SliceEntityOwnershipServiceRequestBus::Events::FindLoadedEntityIdMapping, testEntity->GetId());

        // Verify that the entityId in the loadedEntityIdMap is same as the provided entityId, which happens when remapping is not done.
        EXPECT_TRUE(loadedEntityId == testEntity->GetId());
    }

    TEST_F(SliceEntityOwnershipTests, FindLoadedEntityIdMapping_IdsRemapped_EntityIdAbsent)
    {
        AZ::Entity* rootEntity = aznew AZ::Entity();
        AZ::SliceComponent* rootSliceComponent = rootEntity->CreateComponent<AZ::SliceComponent>();
        AZ::Entity* testEntity = aznew AZ::Entity();
        rootSliceComponent->AddEntity(testEntity);

        AZ::SliceComponent::EntityIdToEntityIdMap previousToNewIdMap;
        previousToNewIdMap.emplace(testEntity->GetId(), testEntity->GetId());
        bool rootEntityLoadSuccessful = false;
        AzFramework::SliceEntityOwnershipServiceRequestBus::BroadcastResult(rootEntityLoadSuccessful,
            &AzFramework::SliceEntityOwnershipServiceRequestBus::Events::HandleRootEntityReloadedFromStream,
            rootEntity, true, &previousToNewIdMap);
        EXPECT_TRUE(rootEntityLoadSuccessful);

        AZ::EntityId loadedEntityId;
        AzFramework::SliceEntityOwnershipServiceRequestBus::BroadcastResult(loadedEntityId,
            &AzFramework::SliceEntityOwnershipServiceRequestBus::Events::FindLoadedEntityIdMapping, testEntity->GetId());

        // Verify that entityId is not present in the loadedEntityIdMap when remapping is done.
        EXPECT_FALSE(loadedEntityId.IsValid());
    }

    TEST_F(SliceEntityOwnershipTests, OnAssetReady_RootSliceAssetReady_DoNotInstantiate)
    {
        m_sliceEntityOwnershipService->OnAssetReady(GetRootSliceAsset());

        // Verify that validate entities callback is not triggered,
        // which will only happen when an attempt to instantiate slice didn't occur.
        EXPECT_FALSE(m_validateEntitiesCallbackTriggered);
    }

    TEST_F(SliceEntityOwnershipTests, OnAssetError_RootSliceAssetError_DoNotClearOtherSliceInstantiations)
    {
        AddSlice(AzFramework::EntityList{}, true);
        m_sliceEntityOwnershipService->OnAssetError(GetRootSliceAsset());

        // Try to finish any queued slice instantiations
        AZ::TickBus::ExecuteQueuedEvents();

        // Verify that slice instantiation was successful.
        size_t slicesCountUnderRootSlice = GetRootSliceAsset()->GetComponent()->GetSlices().size();
        EXPECT_EQ(slicesCountUnderRootSlice, 1);
    }

    TEST_F(SliceEntityOwnershipTests, OnAssetError_InstantiatingAssetError_StopSliceInstantiation)
    {
        AZ::Data::Asset<AZ::SliceAsset> sliceAsset1;
        AZ::Data::AssetId sliceAsset1Id = AZ::Data::AssetId(AZ::Uuid::CreateRandom());
        sliceAsset1.Create(sliceAsset1Id, false);
        AddSlice(AzFramework::EntityList{}, true, sliceAsset1);

        AZ::Data::Asset<AZ::SliceAsset> sliceAsset2;
        sliceAsset2.Create(AZ::Data::AssetId(AZ::Uuid::CreateRandom()), false);
        AddSlice(AzFramework::EntityList{}, true, sliceAsset2);

        m_sliceEntityOwnershipService->OnAssetError(sliceAsset2);

        // Try to finish any queued slice instantiations
        AZ::TickBus::ExecuteQueuedEvents();

        AZ::SliceComponent::SliceList& slicesUnderRootSlice = GetRootSliceAsset()->GetComponent()->GetSlices();

        // Verify that there is only one slice under root slice
        EXPECT_EQ(slicesUnderRootSlice.size(), 1);

        // Verify that the slice without the asset error was instantiated
        EXPECT_EQ(slicesUnderRootSlice.front().GetSliceAsset().GetId(), sliceAsset1Id);
    }

    TEST_F(SliceEntityOwnershipTests, InstantiateSlice_InvalidAssetId_ReturnBlankInstantiationTicket)
    {
        // Set the asset id to null to invalidate it.
        AZ_TEST_START_TRACE_SUPPRESSION;
        AZ::Data::Asset<AZ::SliceAsset> sliceAssetHolder = AZ::Data::AssetManager::Instance().
            CreateAsset<AZ::SliceAsset>(AZ::Data::AssetId{});
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);
        EXPECT_EQ(nullptr, sliceAssetHolder.Get());

        AzFramework::SliceInstantiationTicket sliceInstantiationTicket;
        AzFramework::SliceEntityOwnershipServiceRequestBus::BroadcastResult(sliceInstantiationTicket,
            &AzFramework::SliceEntityOwnershipServiceRequestBus::Events::InstantiateSlice, sliceAssetHolder, nullptr, nullptr);
        AZ::TickBus::ExecuteQueuedEvents();

        // Verify that there is no request id or context id associated with the sliceInstantiationTicket
        EXPECT_EQ(sliceInstantiationTicket.GetContextId(), AZ::Uuid::CreateNull());
        EXPECT_EQ(sliceInstantiationTicket.GetRequestId(), 0);
    }
    
    TEST_F(SliceEntityOwnershipTests, InstantiateSlice_InstantiateTwoSlices_SlicesInstantiated)
    {
        // Add 2 slices asynchronously
        AddSlice(AzFramework::EntityList{}, true);
        AddSlice(AzFramework::EntityList{}, true);
        AZ::TickBus::ExecuteQueuedEvents();
        size_t slicesCountUnderRootSlice = GetRootSliceAsset()->GetComponent()->GetSlices().size();
        EXPECT_EQ(slicesCountUnderRootSlice, 2);
    }

    TEST_F(SliceEntityOwnershipTests, CloneSliceInstance_InstantiateSlice_SliceCloned)
    {
        AZ::Entity* testEntity = aznew AZ::Entity("testEntity");
        AddSlice(AzFramework::EntityList{ testEntity });

        AZ::SliceComponent::EntityIdSet entityIdsInSlice;
        GetRootSliceAsset()->GetComponent()->GetEntityIds(entityIdsInSlice);

        AZ::SliceComponent* rootSlice = nullptr;
        AzFramework::SliceEntityOwnershipServiceRequestBus::BroadcastResult(rootSlice,
            &AzFramework::SliceEntityOwnershipServiceRequestBus::Events::GetRootSlice);
        AZ::SliceComponent::SliceInstanceAddress sourceSliceInstanceAddress = rootSlice->FindSlice(*entityIdsInSlice.begin());
        AZ::SliceComponent::EntityIdToEntityIdMap entityIdToEntityIdMap;
        AZ::SliceComponent::SliceInstanceAddress clonedSliceInstanceAddress;
        AzFramework::SliceEntityOwnershipServiceRequestBus::BroadcastResult(clonedSliceInstanceAddress,
            &AzFramework::SliceEntityOwnershipServiceRequests::CloneSliceInstance, sourceSliceInstanceAddress, entityIdToEntityIdMap);

        // Verify that the entity was cloned successfully with the slice
        EXPECT_EQ(clonedSliceInstanceAddress.GetInstance()->GetInstantiated()->m_entities.front()->GetName(), "testEntity");

        // Verify that the source slice and the cloned slice have the same reference.
        EXPECT_EQ(sourceSliceInstanceAddress.GetReference(), clonedSliceInstanceAddress.GetReference());
    }

    TEST_F(SliceEntityOwnershipTests, InstantiateSlice_EntitiesInvalid_SliceInstantiationFailed)
    {
        m_areEntitiesValidForContext = false;
        AddSlice(AzFramework::EntityList{});

        size_t slicesCountUnderRootSlice = GetRootSliceAsset()->GetComponent()->GetSlices().size();

        // If entities are invalid, then slice instantiation would fail
        EXPECT_EQ(slicesCountUnderRootSlice, 0);
    }

    TEST_F(SliceEntityOwnershipTests, CancelSliceInstantiation_SetupCorrect_SliceInstantiationCanceled)
    {
        AzFramework::SliceInstantiationTicket sliceInstantiationTicket = AddSlice(AzFramework::EntityList{}, true);

        AzFramework::SliceEntityOwnershipServiceRequestBus::Broadcast(
            &AzFramework::SliceEntityOwnershipServiceRequestBus::Events::CancelSliceInstantiation, sliceInstantiationTicket);

        // This will try to finish any queued slice instantiations.
        AZ::TickBus::ExecuteQueuedEvents();

        size_t slicesCountUnderRootSlice = GetRootSliceAsset()->GetComponent()->GetSlices().size();
        EXPECT_EQ(slicesCountUnderRootSlice, 0);
    }

    TEST_F(SliceEntityOwnershipTests, GetOwningSlice_SliceAdded_OwningSliceFetchedCorrectly)
    {
        AZ::SliceComponent::SliceList& slicesUnderRootSlice = GetRootSliceAsset()->GetComponent()->GetSlices();
        AddSlice(AzFramework::EntityList{ aznew AZ::Entity() });
        ASSERT_EQ(slicesUnderRootSlice.size(), 1);
        AZ::SliceComponent::SliceReference& sliceReference = slicesUnderRootSlice.front();
        ASSERT_EQ(sliceReference.GetInstances().size(), 1);
        AzFramework::EntityList entitiesOfSlice = sliceReference.GetInstances().begin()->GetInstantiated()->m_entities;
        ASSERT_EQ(entitiesOfSlice.size(), 1);
        AZ::SliceComponent::SliceInstanceAddress sliceInstanceAddress;
        AzFramework::SliceEntityRequestBus::EventResult(sliceInstanceAddress, entitiesOfSlice.front()->GetId(),
            &AzFramework::SliceEntityRequestBus::Events::GetOwningSlice);

        // Verify that the source slice and the cloned slice have the same slice asset.
        EXPECT_EQ(sliceInstanceAddress.GetReference()->GetSliceAsset(), sliceReference.GetSliceAsset());
    }

    TEST_F(SliceEntityOwnershipTests, GetOwningSlice_LooseEntityAdded_EntityHasNoOwningSlice)
    {
        AZ::Entity* testEntity = aznew AZ::Entity();
        m_sliceEntityOwnershipService->AddEntity(testEntity);
        AZ::SliceComponent::SliceInstanceAddress sliceInstanceAddress;
        AzFramework::SliceEntityRequestBus::EventResult(sliceInstanceAddress, testEntity->GetId(),
            &AzFramework::SliceEntityRequestBus::Events::GetOwningSlice);

        // Verify that the loose entity doesn't belong to a slice instance
        EXPECT_FALSE(sliceInstanceAddress.IsValid());
    }

    TEST_F(SliceEntityOwnershipTests, AddEntity_RootSliceAssetAbsent_EntityNotCreated)
    {
        m_sliceEntityOwnershipService->Destroy();
        AZ::Entity testEntity = AZ::Entity("testEntity");
        AZ_TEST_START_ASSERTTEST;
        m_sliceEntityOwnershipService->AddEntity(&testEntity);
        AZ_TEST_STOP_ASSERTTEST(1); // we expect an assert here but we expect NO death or crash, just a clean return.

    }
}
