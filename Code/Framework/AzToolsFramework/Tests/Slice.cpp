/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Slice/SliceAssetHandler.h>
#include <AzCore/UserSettings/UserSettingsComponent.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <AzToolsFramework/Slice/SliceUtilities.h>
#include <AzToolsFramework/Application/ToolsApplication.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/ToolsComponents/TransformComponent.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
#include <AzToolsFramework/Entity/EditorEntitySortComponent.h>
#include <AzToolsFramework/Entity/SliceEditorEntityOwnershipServiceBus.h>
#include <AzToolsFramework/UI/Slice/SlicePushWidget.hxx>
#include <AzToolsFramework/UnitTest/AzToolsFrameworkTestHelpers.h>
#include <AzToolsFramework/UnitTest/ToolsTestApplication.h>

namespace UnitTest
{
    class SlicePushCyclicDependencyTest
        : public AllocatorsTestFixture
    {
    public:
        SlicePushCyclicDependencyTest()
            : AllocatorsTestFixture()
        { }

        void SetUp() override
        {
            AZ::ComponentApplication::Descriptor componentApplicationDesc;
            componentApplicationDesc.m_useExistingAllocator = true;
            m_application = aznew ToolsTestApplication("SlicePushCyclicDependencyTest");
            m_application->Start(componentApplicationDesc);
            // Without this, the user settings component would attempt to save on finalize/shutdown. Since the file is
            // shared across the whole engine, if multiple tests are run in parallel, the saving could cause a crash 
            // in the unit tests.
            AZ::UserSettingsComponentRequestBus::Broadcast(&AZ::UserSettingsComponentRequests::DisableSaveOnFinalize);
        }

        void TearDown() override
        {
            // Release all slice asset references, so AssetManager doens't complain.
            m_sliceAssets.clear();
            delete m_application;
        }

        // This function transfers the ownership of the argument `entity`. Do not delete or use it afterwards.
        AZ::Data::AssetId SaveAsSlice(AZ::Entity* entity)
        {
            AZStd::vector<AZ::Entity*> entities;
            entities.push_back(entity);
            return SaveAsSlice(entities);
        }

        // This function transfers the ownership of all the entity pointers. Do not delete or use them afterwards.
        AZ::Data::AssetId SaveAsSlice(AZStd::vector<AZ::Entity*> entities)
        {
            AZ::Entity* sliceEntity = aznew AZ::Entity();
            AZ::SliceComponent* sliceComponent = nullptr;
            sliceComponent = aznew AZ::SliceComponent();
            sliceComponent->SetSerializeContext(m_application->GetSerializeContext());
            for (auto& entity : entities)
            {
                sliceComponent->AddEntity(entity);
            }

            // Don't activate `sliceEntity`, whose purpose is to be attached by `sliceComponent`.
            sliceEntity->AddComponent(sliceComponent);

            AZ::Data::AssetId assetId = AZ::Data::AssetId(AZ::Uuid::CreateRandom(), 0);
            AZ::Data::Asset<AZ::SliceAsset> sliceAssetHolder = AZ::Data::AssetManager::Instance().CreateAsset<AZ::SliceAsset>(assetId, AZ::Data::AssetLoadBehavior::Default);
            sliceAssetHolder.GetAs<AZ::SliceAsset>()->SetData(sliceEntity, sliceComponent);

            // Hold on to sliceAssetHolder so it's not ref-counted away.
            m_sliceAssets.emplace(assetId, sliceAssetHolder);

            return assetId;
        }

        AZ::SliceComponent::EntityList InstantiateSlice(AZ::Data::AssetId sliceAssetId)
        {
            auto foundItr = m_sliceAssets.find(sliceAssetId);
            AZ_TEST_ASSERT(foundItr != m_sliceAssets.end());

            AZ::SliceComponent* rootSlice;
            AzToolsFramework::SliceEditorEntityOwnershipServiceRequestBus::BroadcastResult(rootSlice,
                &AzToolsFramework::SliceEditorEntityOwnershipServiceRequestBus::Events::GetEditorRootSlice);
            AZ::SliceComponent::SliceInstanceAddress sliceInstAddress = rootSlice->AddSlice(foundItr->second);
            rootSlice->Instantiate();

            const AZ::SliceComponent::InstantiatedContainer* instanceContainer = sliceInstAddress.GetInstance()->GetInstantiated();
            AzToolsFramework::EditorEntityContextRequestBus::Broadcast(&AzToolsFramework::EditorEntityContextRequestBus::Events::HandleEntitiesAdded, instanceContainer->m_entities);

            return instanceContainer->m_entities;
        }

        void RemoveAllSlices()
        {
            AZ::SliceComponent* rootSlice;
            AzToolsFramework::SliceEditorEntityOwnershipServiceRequestBus::BroadcastResult(rootSlice,
                &AzToolsFramework::SliceEditorEntityOwnershipServiceRequestBus::Events::GetEditorRootSlice);
            for (auto sliceAssetPair : m_sliceAssets)
            {
                rootSlice->RemoveSlice(sliceAssetPair.second);
            }
        }

    public:
        AZ::IO::LocalFileIO m_localFileIO;
        ToolsTestApplication* m_application = nullptr;

        AZStd::unordered_map<AZ::Data::AssetId, AZ::Data::Asset<AZ::SliceAsset>> m_sliceAssets;
    };

    // Test pushing slices to create news slices that could result in cyclic 
    // dependency, e.g. push slice1 => slice2 and slice2 => slice1 at the same
    // time.
    TEST_F(SlicePushCyclicDependencyTest, PushTwoSlicesToDependOnEachOther)
    {
        AUTO_RESULT_IF_SETTING_TRUE(UnitTest::prefabSystemSetting, true)

        AZ::Entity* entity = aznew AZ::Entity("TestEntity0");
        entity->CreateComponent<AzToolsFramework::Components::TransformComponent>();
        AZ::Data::AssetId sliceAssetId0 = SaveAsSlice(entity);
        entity = nullptr;

        entity = aznew AZ::Entity("TestEntity1");
        entity->CreateComponent<AzToolsFramework::Components::TransformComponent>();
        AZ::Data::AssetId sliceAssetId1 = SaveAsSlice(entity);
        entity = nullptr;

        AZ::SliceComponent::EntityList slice0EntitiesA = InstantiateSlice(sliceAssetId0);
        EXPECT_EQ(slice0EntitiesA.size(), 1);
        AZ::SliceComponent::EntityList slice0EntitiesB = InstantiateSlice(sliceAssetId0);
        EXPECT_EQ(slice0EntitiesB.size(), 1);

        AZ::SliceComponent::EntityList slice1EntitiesA = InstantiateSlice(sliceAssetId1);
        EXPECT_EQ(slice1EntitiesA.size(), 1);
        AZ::SliceComponent::EntityList slice1EntitiesB = InstantiateSlice(sliceAssetId1);
        EXPECT_EQ(slice1EntitiesA.size(), 1);

        // Reparent entities to slice1EntityA <-- slice0EntityA, slice0EntityB <-- slice1EntityA (<-- points to parent).
        AZ::TransformBus::Event(slice0EntitiesA[0]->GetId(), &AZ::TransformBus::Events::SetParent, slice1EntitiesA[0]->GetId());
        AZ::TransformBus::Event(slice1EntitiesB[0]->GetId(), &AZ::TransformBus::Events::SetParent, slice0EntitiesB[0]->GetId());

        AZStd::unordered_map<AZ::Data::AssetId, AZ::SliceComponent::EntityIdSet> unpushableEntityIdsPerAsset;
        AZStd::unordered_map<AZ::EntityId, AZ::SliceComponent::EntityAncestorList> sliceAncestryMapping;
        AZStd::vector<AZStd::pair<AZ::EntityId, AZ::SliceComponent::EntityAncestorList>> newChildEntityIdAncestorPairs;

        AZStd::unordered_set<AZ::EntityId> entitiesToAdd;
        AzToolsFramework::EntityIdList inputEntityIds = { slice0EntitiesA[0]->GetId(), slice0EntitiesB[0]->GetId(), slice1EntitiesA[0]->GetId(), slice1EntitiesB[0]->GetId() };
        AZStd::unordered_set<AZ::EntityId> pushableNewChildEntityIds = AzToolsFramework::SliceUtilities::GetPushableNewChildEntityIds(
            inputEntityIds, unpushableEntityIdsPerAsset, sliceAncestryMapping, newChildEntityIdAncestorPairs, entitiesToAdd);

        // Because there would be cyclic dependency in the resulting slices, we only allow pushing of one entity.
        AZ_TEST_ASSERT(unpushableEntityIdsPerAsset.size() == 1);
        AZ_TEST_ASSERT(newChildEntityIdAncestorPairs.size() == 1);

        RemoveAllSlices();
    }

    TEST_F(SlicePushCyclicDependencyTest, PushMultipleEntitiesOneOfChildrenCauseCyclicDependency)
    {
        AUTO_RESULT_IF_SETTING_TRUE(UnitTest::prefabSystemSetting, true)

        AZ::Entity* tempAssetEntity = aznew AZ::Entity("TestEntity0");
        tempAssetEntity->CreateComponent<AzToolsFramework::Components::TransformComponent>();
        AZ::Data::AssetId sliceAssetId0 = SaveAsSlice(tempAssetEntity);
        tempAssetEntity = nullptr;

        AZ::SliceComponent::EntityList slice0EntitiesA = InstantiateSlice(sliceAssetId0);
        EXPECT_EQ(slice0EntitiesA.size(), 1);
        AZ::SliceComponent::EntityList slice0EntitiesB = InstantiateSlice(sliceAssetId0);
        EXPECT_EQ(slice0EntitiesB.size(), 1);

        AZ::Entity* looseEntity0 = aznew AZ::Entity("LooseEntity");
        looseEntity0->CreateComponent<AzToolsFramework::Components::TransformComponent>();
        AzToolsFramework::EditorEntityContextRequestBus::Broadcast(&AzToolsFramework::EditorEntityContextRequestBus::Events::AddEditorEntity, looseEntity0);

        // Add one pushable entity as a parent of the one that will cause cyclic dependency.
        AZ::TransformBus::Event(looseEntity0->GetId(), &AZ::TransformBus::Events::SetParent, slice0EntitiesA[0]->GetId());
        AZ::TransformBus::Event(slice0EntitiesB[0]->GetId(), &AZ::TransformBus::Events::SetParent, looseEntity0->GetId());

        AZ::SliceComponent::EntityIdSet unpushableEntityIds;
        AZStd::unordered_set<AZ::EntityId> entitiesToAdd;
        AZStd::unordered_map<AZ::Data::AssetId, AZ::SliceComponent::EntityIdSet> unpushableEntityIdsPerAsset;
        AZStd::unordered_map<AZ::EntityId, AZ::SliceComponent::EntityAncestorList> sliceAncestryMapping;
        AZStd::vector<AZStd::pair<AZ::EntityId, AZ::SliceComponent::EntityAncestorList>> newChildEntityIdAncestorPairs;

        AzToolsFramework::EntityIdList inputEntityIds = { slice0EntitiesA[0]->GetId(), slice0EntitiesB[0]->GetId(), looseEntity0->GetId() };
        AZStd::unordered_set<AZ::EntityId> pushableNewChildEntityIds = AzToolsFramework::SliceUtilities::GetPushableNewChildEntityIds(
            inputEntityIds, unpushableEntityIdsPerAsset, sliceAncestryMapping, newChildEntityIdAncestorPairs, entitiesToAdd);

        // slice0EntityB can't be pushed to slice0EntityA, but its parent (looseEntity) can.
        AZ_TEST_ASSERT(unpushableEntityIdsPerAsset.size() == 1);
        AZ_TEST_ASSERT(newChildEntityIdAncestorPairs.size() == 1);

        AZ::Entity* looseEntity1 = aznew AZ::Entity("LooseEntity");
        looseEntity1->CreateComponent<AzToolsFramework::Components::TransformComponent>();
        AzToolsFramework::EditorEntityContextRequestBus::Broadcast(&AzToolsFramework::EditorEntityContextRequestBus::Events::AddEditorEntity, looseEntity1);

        // Add one more pushable entity as a parent.
        AZ::TransformBus::Event(slice0EntitiesB[0]->GetId(), &AZ::TransformBus::Events::SetParent, looseEntity1->GetId());
        AZ::TransformBus::Event(looseEntity1->GetId(), &AZ::TransformBus::Events::SetParent, looseEntity0->GetId());

        inputEntityIds.push_back(looseEntity1->GetId());
        unpushableEntityIds.clear();
        sliceAncestryMapping.clear();
        newChildEntityIdAncestorPairs.clear();
        pushableNewChildEntityIds = AzToolsFramework::SliceUtilities::GetPushableNewChildEntityIds(inputEntityIds,
            unpushableEntityIdsPerAsset, sliceAncestryMapping, newChildEntityIdAncestorPairs, entitiesToAdd);

        // slice0EntityB can't be pushed to slice0EntityA, but the two LooseEntity instances can
        AZ_TEST_ASSERT(unpushableEntityIdsPerAsset.size() == 1);
        AZ_TEST_ASSERT(newChildEntityIdAncestorPairs.size() == 2);

        tempAssetEntity = aznew AZ::Entity("TestEntity1");
        tempAssetEntity->CreateComponent<AzToolsFramework::Components::TransformComponent>();
        SaveAsSlice(tempAssetEntity);
        tempAssetEntity = nullptr;

        AZ::SliceComponent::EntityList slice1EntitiesA = InstantiateSlice(sliceAssetId0);
        EXPECT_EQ(slice1EntitiesA.size(), 1);

        // Add another slice-owned entity `slice1EntitiesA` as the parent of the one causing cyclic dependency, 
        // and push addition of `slice1EntitiesA`.
        AZ::TransformBus::Event(slice0EntitiesB[0]->GetId(), &AZ::TransformBus::Events::SetParent, slice1EntitiesA[0]->GetId());
        AZ::TransformBus::Event(slice1EntitiesA[0]->GetId(), &AZ::TransformBus::Events::SetParent, slice0EntitiesA[0]->GetId());

        inputEntityIds.clear();
        inputEntityIds.push_back(slice0EntitiesA[0]->GetId());
        inputEntityIds.push_back(slice0EntitiesB[0]->GetId());
        inputEntityIds.push_back(slice1EntitiesA[0]->GetId());
        unpushableEntityIds.clear();
        sliceAncestryMapping.clear();
        newChildEntityIdAncestorPairs.clear();
        pushableNewChildEntityIds = AzToolsFramework::SliceUtilities::GetPushableNewChildEntityIds(inputEntityIds, 
            unpushableEntityIdsPerAsset, sliceAncestryMapping, newChildEntityIdAncestorPairs, entitiesToAdd);

        AZ_TEST_ASSERT(unpushableEntityIdsPerAsset.size() == 1);
        if (unpushableEntityIdsPerAsset.size() == 1)
        {
            AzToolsFramework::EntityIdSet ids = unpushableEntityIdsPerAsset.begin()->second;
            AZ_TEST_ASSERT(ids.size() == 2);
        }
        AZ_TEST_ASSERT(newChildEntityIdAncestorPairs.size() == 0);

        // But if an entity is not a parent of an unpushable one, it should be added.
        AZ::TransformBus::Event(looseEntity0->GetId(), &AZ::TransformBus::Events::SetParent, slice0EntitiesA[0]->GetId());

        inputEntityIds.push_back(looseEntity0->GetId());
        unpushableEntityIds.clear();
        sliceAncestryMapping.clear();
        newChildEntityIdAncestorPairs.clear();
        pushableNewChildEntityIds = AzToolsFramework::SliceUtilities::GetPushableNewChildEntityIds(inputEntityIds, 
            unpushableEntityIdsPerAsset, sliceAncestryMapping, newChildEntityIdAncestorPairs, entitiesToAdd);

        AZ_TEST_ASSERT(unpushableEntityIdsPerAsset.size() == 1);
        if (unpushableEntityIdsPerAsset.size() == 1)
        {
            AzToolsFramework::EntityIdSet ids = unpushableEntityIdsPerAsset.begin()->second;
            AZ_TEST_ASSERT(ids.size() == 2);
        }
        AZ_TEST_ASSERT(newChildEntityIdAncestorPairs.size() == 1);

        RemoveAllSlices();
    }

    TEST_F(SlicePushCyclicDependencyTest, PushSliceWithNewDuplicatedChild)
    {
        AUTO_RESULT_IF_SETTING_TRUE(UnitTest::prefabSystemSetting, true)

        AZ::Entity* entity = aznew AZ::Entity("TestEntity0");
        entity->CreateComponent<AzToolsFramework::Components::TransformComponent>();
        AZ::Data::AssetId sliceAssetId0 = SaveAsSlice(entity);
        entity = nullptr;

        entity = aznew AZ::Entity("TestEntity1");
        entity->CreateComponent<AzToolsFramework::Components::TransformComponent>();
        AZ::Data::AssetId sliceAssetId1 = SaveAsSlice(entity);
        entity = nullptr;

        AZ::SliceComponent::EntityList slice0Entities = InstantiateSlice(sliceAssetId0);
        EXPECT_EQ(slice0Entities.size(), 1);
        AZ::SliceComponent::EntityList slice1EntitiesA = InstantiateSlice(sliceAssetId1);
        EXPECT_EQ(slice1EntitiesA.size(), 1);
        AZ::SliceComponent::EntityList slice1EntitiesB = InstantiateSlice(sliceAssetId1);
        EXPECT_EQ(slice1EntitiesB.size(), 1);

        // Reparent the entity1s to be children of entity0
        AZ::TransformBus::Event(slice1EntitiesA[0]->GetId(), &AZ::TransformBus::Events::SetParent, slice0Entities[0]->GetId());
        AZ::TransformBus::Event(slice1EntitiesB[0]->GetId(), &AZ::TransformBus::Events::SetParent, slice0Entities[0]->GetId());


        AZStd::unordered_set<AZ::EntityId> entitiesToAdd;
        AZStd::unordered_map<AZ::Data::AssetId, AZ::SliceComponent::EntityIdSet> unpushableEntityIdsPerAsset;
        AZStd::unordered_map<AZ::EntityId, AZ::SliceComponent::EntityAncestorList> sliceAncestryMapping;
        AZStd::vector<AZStd::pair<AZ::EntityId, AZ::SliceComponent::EntityAncestorList>> newChildEntityIdAncestorPairs;

        AzToolsFramework::EntityIdList inputEntityIds = { slice0Entities[0]->GetId(), slice1EntitiesA[0]->GetId(), slice1EntitiesB[0]->GetId() };
        AZStd::unordered_set<AZ::EntityId> pushableNewChildEntityIds = AzToolsFramework::SliceUtilities::GetPushableNewChildEntityIds(
            inputEntityIds, unpushableEntityIdsPerAsset, sliceAncestryMapping, newChildEntityIdAncestorPairs, entitiesToAdd);

        // Because there would be cyclic dependency in the resulting slices, we only allow pushing of one entity.
        AZ_TEST_ASSERT(newChildEntityIdAncestorPairs.size() == 2);
        AZ_TEST_ASSERT(unpushableEntityIdsPerAsset.size() == 0);
        AZ_TEST_ASSERT(newChildEntityIdAncestorPairs.size() == 2);

        RemoveAllSlices();
    }

    // Test pushing slice with children that aren't going to be in the pushed version
    // either because the user has chosen to leave them out, or they are unpushable for some reason
    // (e.g. they would create a circular dependency).
    TEST_F(SlicePushCyclicDependencyTest, SlicePush_DontPushSomeChildren_ChildrenRemovedFromChildOrderArray)
    {
        AUTO_RESULT_IF_SETTING_TRUE(UnitTest::prefabSystemSetting, true)

        AZ::Data::AssetManager& assetManager = AZ::Data::AssetManager::Instance();
        
        // Create a slice
        AZ::Entity* entity = aznew AZ::Entity("TestEntity0");
        entity->CreateComponent<AzToolsFramework::Components::TransformComponent>();
        AZ::Data::AssetId sliceAssetId0 = SaveAsSlice(entity);
        entity = nullptr;

        // Instantiate two copies of the slice.
        AZ::SliceComponent::EntityList parentSlice = InstantiateSlice(sliceAssetId0);
        AZ::SliceComponent::EntityList childSlice = InstantiateSlice(sliceAssetId0);

        // Make one a child of the other.
        AZ::TransformBus::Event(childSlice[0]->GetId(), &AZ::TransformBus::Events::SetParent, parentSlice[0]->GetId());

        // Grab the parent entity and add an EditorEntitySortComponent to it. 
        AzToolsFramework::Components::EditorEntitySortComponent* parentSortComponent;
        AZ::Entity* parentEntity = nullptr;
        {
            AZ::ComponentApplicationBus::BroadcastResult(parentEntity, &AZ::ComponentApplicationBus::Handler::FindEntity, parentSlice[0]->GetId());
            AZ_Assert(parentEntity, "Failed to find parentEntity\n");
            parentEntity->Deactivate();
            parentSortComponent = parentEntity->CreateComponent<AzToolsFramework::Components::EditorEntitySortComponent>();
            AZ_Assert(parentSortComponent, "Failed to create parentSortComponent\n");
            parentEntity->Activate();
        }

        // Create two entities and make them children of the parent
        AZ::Entity* childEntity0;
        {
            childEntity0 = aznew AZ::Entity("TestChildEntity");
            childEntity0->CreateComponent<AzToolsFramework::Components::TransformComponent>();
            childEntity0->Init();
            childEntity0->Activate();
            AZ::TransformBus::Event(childEntity0->GetId(), &AZ::TransformBus::Events::SetParent, parentEntity->GetId());
            AZ_Assert(childEntity0, "Failed to create childEntity0\n");
        }

        AZ::Entity* childEntity1;
        {
            childEntity1 = aznew AZ::Entity("TestChildEntity");
            childEntity1->CreateComponent<AzToolsFramework::Components::TransformComponent>();
            childEntity1->Init();
            childEntity1->Activate();
            AZ::TransformBus::Event(childEntity1->GetId(), &AZ::TransformBus::Events::SetParent, parentEntity->GetId());
            AZ_Assert(childEntity1, "Failed to create childEntity0\n");
        }

        // Analyse hierarchy for unpushable entities.
        AZStd::unordered_map<AZ::Data::AssetId, AZ::SliceComponent::EntityIdSet> unpushableEntityIdsPerAsset;
        {
            AZStd::unordered_map<AZ::EntityId, AZ::SliceComponent::EntityAncestorList> sliceAncestryMapping;
            AZStd::vector<AZStd::pair<AZ::EntityId, AZ::SliceComponent::EntityAncestorList>> newChildEntityIdAncestorPairs;

            AZStd::unordered_set<AZ::EntityId> entitiesToAdd;
            // Make list of entities to be pushed. Leave out childEntity1 to emulate a user having unchecked it in the advanced push widget.
            AzToolsFramework::EntityIdList inputEntityIds = { parentEntity->GetId(), childSlice[0]->GetId(), childEntity0->GetId() };
            AZStd::unordered_set<AZ::EntityId> pushableNewChildEntityIds = AzToolsFramework::SliceUtilities::GetPushableNewChildEntityIds(
                inputEntityIds, unpushableEntityIdsPerAsset, sliceAncestryMapping, newChildEntityIdAncestorPairs, entitiesToAdd);

            // UnpushableEntityIdsPerAsset should now contain a reference to childSlice which can't be
            // pushed as it would create a circular reference. This would get picked up by advanced or quick push
            // during GetPushableNewChildEntityIds.
            AZ_TEST_ASSERT(unpushableEntityIdsPerAsset.size() == 1);
        }
        
        // Add all child entities to the parent slice's child order array.
        parentSortComponent->AddChildEntity(childSlice[0]->GetId(), false);
        parentSortComponent->AddChildEntity(childEntity0->GetId(), false);
        parentSortComponent->AddChildEntity(childEntity1->GetId(), false);
        AzToolsFramework::EntityOrderArray orderArray = parentSortComponent->GetChildEntityOrderArray();

        // Make a list of entities that we don't want to push (childEntity1). This will emulate a user deciding not to push
        // certain entities in the advanced push widget.
        AZStd::vector <AZ::EntityId> idsNotToPush;
        idsNotToPush.push_back(childEntity1->GetId());

        // Do the pruning to produce the list of entities that will be pushed.
        AzToolsFramework::EntityOrderArray prunedOrderArray;
        {
            prunedOrderArray.reserve(orderArray.size());

            AzToolsFramework::SliceUtilities::WillPushEntityCallback willPushEntityCallback =
                [&unpushableEntityIdsPerAsset, &idsNotToPush]
            (const AZ::EntityId entityId, const  AZ::Data::Asset <AZ::SliceAsset>& assetToPushTo) -> bool
            {
                if (unpushableEntityIdsPerAsset[assetToPushTo.GetId()].find(entityId) != unpushableEntityIdsPerAsset[assetToPushTo.GetId()].end())
                {
                    return false;
                }

                for (AZ::EntityId id : idsNotToPush)
                {
                    if (id == entityId)
                    {
                        return false;
                    }
                }

                return true;
            };

            AZ::Data::Asset<AZ::SliceAsset> sliceAsset = assetManager.FindOrCreateAsset<AZ::SliceAsset>(sliceAssetId0, AZ::Data::AssetLoadBehavior::Default);

            AzToolsFramework::SliceUtilities::RemoveInvalidChildOrderArrayEntries(orderArray, prunedOrderArray, sliceAsset, willPushEntityCallback);
        }

        // At this point there should only be childEntity0 in the pruned order array.
        bool pruningCorrect = false;

        if (prunedOrderArray.size() == 1 && prunedOrderArray[0] == childEntity0->GetId())
        {
            pruningCorrect = true;
        }

        EXPECT_EQ(pruningCorrect, true);

        RemoveAllSlices();
    }

    // Rename our fixture class for the next test so that it has a more accurate test name.
    class SliceActivationOrderTest : public SlicePushCyclicDependencyTest {};

    // Class that listens for AZ_Warning messages and asserts if any are found.
    class SliceTestWarningInterceptor :
        public AZ::Debug::TraceMessageBus::Handler
    {
        public:
            SliceTestWarningInterceptor()
            {
                AZ::Debug::TraceMessageBus::Handler::BusConnect();
            }
            ~SliceTestWarningInterceptor() override
            {
                AZ::Debug::TraceMessageBus::Handler::BusDisconnect();
            }
            bool OnWarning(const char *window, const char* message) override
            {
                (void)window;
                ADD_FAILURE() << "Test failed due to an undesirable warning being generated:\n" << message;
                return true;
            }
    };


    // LY-95800:  If a child entity with a transform is present in a slice asset earlier 
    // than its parent, the activation of the parent entity can cause the child to have a
    // state that doesn't match the undo cache, which generates a warning about inconsistent data.
    // (See PreemptiveUndoCache::Validate)
    // If the bug is present, a warning will be thrown which fails this unit test.
    TEST_F(SliceActivationOrderTest, ActivationOrderShouldNotAffectUndoCache)
    {
        AUTO_RESULT_IF_SETTING_TRUE(UnitTest::prefabSystemSetting, true)

        // Swallow deprecation warnings from the Transform component as they are not relevant to this test
        UnitTest::ErrorHandler errorHandler("GetScale is deprecated");

        // Create a parent entity with a transform component
        AZ::Entity* parentEntity = aznew AZ::Entity("TestParentEntity");
        parentEntity->CreateComponent<AzToolsFramework::Components::TransformComponent>();
        parentEntity->Init();
        parentEntity->Activate();

        // Create a child entity with a transform component
        AZ::Entity* childEntity = aznew AZ::Entity("TestChildEntity");
        childEntity->CreateComponent<AzToolsFramework::Components::TransformComponent>();
        childEntity->Init();
        childEntity->Activate();

        // Make the child an actual child of the parent entity
        AZ::TransformBus::Event(childEntity->GetId(), &AZ::TransformBus::Events::SetParent, parentEntity->GetId());

        AZStd::vector<AZ::Entity*> entities;

        // Add our entities to the list of entities to make a slice from.
        // IMPORTANT:  The child should be added before the parent.  For this bug to manifest, the
        // child entity needs to get instantiated and activated before the parent when instantiating
        // the slice.
        childEntity->Deactivate();
        parentEntity->Deactivate();
        entities.push_back(childEntity);
        entities.push_back(parentEntity);

        // When saving a slice, SliceUtilities::VerifyAndApplySliceWorldTransformRules() clears out the 
        // cached world transforms prior to writing out the slice asset.
        for (AZ::Entity* entity : entities)
        {
            AzToolsFramework::Components::TransformComponent* transformComponent = entity->FindComponent<AzToolsFramework::Components::TransformComponent>();
            if (transformComponent)
            {
                transformComponent->ClearCachedWorldTransform();
            }
        }

        // Create our slice asset
        AZ::Data::AssetId sliceAssetId = SaveAsSlice(entities);
        childEntity = nullptr;
        parentEntity = nullptr;
        entities.clear();

        // Create an undo batch to wrap the slice instantiation.
        // This is necessary, because ending the undo batch is what causes the batch to get validated.
        AzToolsFramework::ToolsApplicationRequests::Bus::Broadcast(&AzToolsFramework::ToolsApplicationRequests::Bus::Events::BeginUndoBatch, "Slice Instantiation");

        // Instantiate the slice.
        // This will instantiate the child, save it in the undo batch, instantiate the parent,
        // save the parent in the undo batch, and modify the child.
        // If the bug exists, this will cause the child's undo batch record to become inconsistent,
        // which will cause a warning when we call EndUndoBatch.
        // If the bug is fixed, the child's undo batch record will be updated.
        AZ::SliceComponent::EntityList sliceEntities = InstantiateSlice(sliceAssetId);

        // When instantiating a slice, SliceEditorEntityOwnershipService::OnSliceInstantiated() removes any entities 
        // in the slice from the dirty entity list.  This step is important because in the buggy case, the child 
        // will be marked dirty above, but won't be updated in the undo cache yet.  Removing it ensures it never 
        // will be.  If it isn't removed, it will get updated as a dirty entity when the undo batch ends.
        for (AZ::Entity* entity : sliceEntities)
        {
            AzToolsFramework::ToolsApplicationRequests::Bus::Broadcast(&AzToolsFramework::ToolsApplicationRequests::RemoveDirtyEntity, entity->GetId());
        }

        // End the slice instantiation undo batch.
        // At this point, if the child entity's undo record doesn't match the current child entity, a warning will be emitted.
        {
            // The point of this test is to determine whether or not we got a warning from PreemptiveUndoCache
            // about inconsistent undo data.  So intercept warnings during this step and fail the test if we get one.
            SliceTestWarningInterceptor warningInterceptor;

            AzToolsFramework::ToolsApplicationRequests::Bus::Broadcast(&AzToolsFramework::ToolsApplicationRequests::Bus::Events::EndUndoBatch);
        }

        RemoveAllSlices();
    }

    class SlicePushWidgetTest : public SlicePushCyclicDependencyTest {};

    TEST_F(SlicePushWidgetTest, SlicePushWidget_CalculateLevelReferences_ReferenceCountCorrect)
    {
        AUTO_RESULT_IF_SETTING_TRUE(UnitTest::prefabSystemSetting, true)

        // Create an entities and make it a slice.
        AZ::Entity* entity0 = aznew AZ::Entity("TestEntity0");
        entity0->CreateComponent<AzToolsFramework::Components::TransformComponent>();
        AZ::Data::AssetId sliceAssetIdChild = SaveAsSlice(entity0);

        // Instantiate 5 copies.
        AZ::SliceComponent::EntityList slice0EntitiesA = InstantiateSlice(sliceAssetIdChild);
        AZ::SliceComponent::EntityList slice0EntitiesB = InstantiateSlice(sliceAssetIdChild);
        AZ::SliceComponent::EntityList slice0EntitiesC = InstantiateSlice(sliceAssetIdChild);
        AZ::SliceComponent::EntityList slice0EntitiesD = InstantiateSlice(sliceAssetIdChild);
        AZ::SliceComponent::EntityList slice0EntitiesE = InstantiateSlice(sliceAssetIdChild);

        // Make an entity to parent the slice instances
        AZ::Entity* parent0 = aznew AZ::Entity("TestParent0");
        parent0->CreateComponent<AzToolsFramework::Components::TransformComponent>();
        parent0->Init();
        parent0->Activate();

        AZ::TransformBus::Event(slice0EntitiesA[0]->GetId(), &AZ::TransformBus::Events::SetParent, parent0->GetId());
        AZ::TransformBus::Event(slice0EntitiesB[0]->GetId(), &AZ::TransformBus::Events::SetParent, parent0->GetId());
        AZ::TransformBus::Event(slice0EntitiesC[0]->GetId(), &AZ::TransformBus::Events::SetParent, parent0->GetId());
        AZ::TransformBus::Event(slice0EntitiesD[0]->GetId(), &AZ::TransformBus::Events::SetParent, parent0->GetId());
        AZ::TransformBus::Event(slice0EntitiesE[0]->GetId(), &AZ::TransformBus::Events::SetParent, parent0->GetId());
        
        // Save parent as a slice.
        AZ::Data::AssetId sliceAssetIdParent = SaveAsSlice(parent0);
        AZ::SliceComponent::EntityList slice2EntitiesA = InstantiateSlice(sliceAssetIdParent);

        // Make another parent entity and add a sixth instance of the child slice.
         AZ::Entity* parent1 = aznew AZ::Entity("TestParent1");
        parent1->CreateComponent<AzToolsFramework::Components::TransformComponent>();
        parent1->Init();
        parent1->Activate();

        AZ::SliceComponent::EntityList slice0EntitiesF = InstantiateSlice(sliceAssetIdChild);
        AZ::TransformBus::Event(slice0EntitiesF[0]->GetId(), &AZ::TransformBus::Events::SetParent, parent1->GetId());

        AZ::SliceComponent* rootSlice;
        AzToolsFramework::SliceEditorEntityOwnershipServiceRequestBus::BroadcastResult(rootSlice, &AzToolsFramework::SliceEditorEntityOwnershipServiceRequestBus::Events::GetEditorRootSlice);

        size_t parentSliceCount = AzToolsFramework::SlicePushWidget::CalculateReferenceCount(sliceAssetIdParent, rootSlice);
        size_t childSliceCount = AzToolsFramework::SlicePushWidget::CalculateReferenceCount(sliceAssetIdChild, rootSlice);

        EXPECT_EQ(parentSliceCount, 1);
        EXPECT_EQ(childSliceCount, 6);

        RemoveAllSlices();
    }

}
