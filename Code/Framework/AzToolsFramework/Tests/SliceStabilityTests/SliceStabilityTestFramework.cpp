/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Tests/SliceStabilityTests/SliceStabilityTestFramework.h>

#include <AzCore/Serialization/Utils.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Slice/SliceAsset.h>
#include <AzCore/UserSettings/UserSettingsComponent.h>

#include <AzFramework/Asset/AssetCatalogBus.h>

#include <AzToolsFramework/ToolsComponents/TransformComponent.h>
#include <AzToolsFramework/Slice/SliceUtilities.h>
#include <AzToolsFramework/Asset/AssetSystemComponent.h>
#include <AzToolsFramework/Entity/EditorEntityContextComponent.h>
#include <AzToolsFramework/Entity/EditorEntityHelpers.h>
#include <AzToolsFramework/Entity/EditorEntitySortComponent.h>

namespace UnitTest
{
    void SliceStabilityTest::SetUpEditorFixtureImpl()
    {
        auto* app = GetApplication();
        ASSERT_TRUE(app);

        // Get the serialize context to reflect our types and set our validator's serialize context
        AZ::SerializeContext* serializeContext = app->GetSerializeContext();

        m_validator.SetSerializeContext(serializeContext);

        app->RegisterComponentDescriptor(EntityReferenceComponent::CreateDescriptor());

        // Grab the system entity from the component application
        AZ::Entity* systemEntity = app->FindEntity(AZ::SystemEntityId);

        // Deactivate the AssetSystemComponent
        // We will be implementing the AssetSystemRequestBus and want to avoid Ebus connection conflicts
        AzToolsFramework::AssetSystem::AssetSystemComponent* assetSystemComponent = systemEntity->FindComponent<AzToolsFramework::AssetSystem::AssetSystemComponent>();
        assetSystemComponent->Deactivate();

        AzToolsFramework::AssetSystemRequestBus::Handler::BusConnect();
        AzToolsFramework::EditorRequestBus::Handler::BusConnect();

        AZ::UserSettingsComponentRequestBus::Broadcast(&AZ::UserSettingsComponentRequests::DisableSaveOnFinalize);

        // Cache the existing file io instance and build our mock file io
        m_priorFileIO = AZ::IO::FileIOBase::GetInstance();
        m_fileIOMock = AZStd::make_unique<testing::NiceMock<AZ::IO::MockFileIOBase>>();

        // Swap out current file io instance for our mock
        AZ::IO::FileIOBase::SetInstance(nullptr);
        AZ::IO::FileIOBase::SetInstance(m_fileIOMock.get());

        // Setup the default returns for our mock file io calls
        AZ::IO::MockFileIOBase::InstallDefaultReturns(*m_fileIOMock.get());

        // For write we set the default of the 4th param (bytesWritten) to 1
        // otherwise slice transaction errors out during the mock write for writing the default 0 bytes
        ON_CALL(*m_fileIOMock.get(), Write(testing::_, testing::_, testing::_, testing::_))
            .WillByDefault(
            testing::DoAll(
                testing::SetArgPointee<3>(1),
                testing::Return(AZ::IO::Result(AZ::IO::ResultCode::Success))));

        ON_CALL(*m_fileIOMock.get(), GetAlias(testing::_))
            .WillByDefault(
                testing::Return(""));

        ON_CALL(*m_fileIOMock.get(), Rename(testing::_, testing::_))
            .WillByDefault(
                testing::Return(AZ::IO::Result(AZ::IO::ResultCode::Success)));
    }

    void SliceStabilityTest::TearDownEditorFixtureImpl()
    {
        // Get the system entity from the component application
        AZ::Entity* systemEntity = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(systemEntity, &AZ::ComponentApplicationBus::Events::FindEntity, AZ::SystemEntityId);

        // Deactivate the EditorEntityContextComponent
        // This triggers the entity context to destroy its root slice asset which destroys all entities, slice instances, and meta data entities
        AzToolsFramework::EditorEntityContextComponent* editorEntityContext = systemEntity->FindComponent<AzToolsFramework::EditorEntityContextComponent>();
        editorEntityContext->Deactivate();

        // Restore our original file io instance
        AZ::IO::FileIOBase::SetInstance(nullptr);
        AZ::IO::FileIOBase::SetInstance(m_priorFileIO);

        AzToolsFramework::EditorRequestBus::Handler::BusDisconnect();
        AzToolsFramework::AssetSystemRequestBus::Handler::BusDisconnect();
    }

    AZ::EntityId SliceStabilityTest::CreateEditorEntity(const char* entityName, AzToolsFramework::EntityIdList& entityList, const AZ::EntityId& parentId /*= AZ::EntityId()*/)
    {
        // Start by creating and registering a new loose entity with the editor entity context
        // This call also adds required components onto the entity
        AZ::EntityId newEntityId;
        AzToolsFramework::EditorEntityContextRequestBus::BroadcastResult(
            newEntityId, &AzToolsFramework::EditorEntityContextRequestBus::Events::CreateNewEditorEntity, entityName);

        AZ::Entity* newEntity = AzToolsFramework::GetEntityById(newEntityId);

        // If newEntity is nullptr still then there was a failure in the above EBus call and we cannot proceed
        if (!newEntity)
        {
            return AZ::EntityId();
        }

        // Add to our entities container
        entityList.emplace_back(newEntity->GetId());

        // Get the new entity's transform component
        AzToolsFramework::Components::TransformComponent* entityTransform =
            newEntity->FindComponent<AzToolsFramework::Components::TransformComponent>();

        // If new entity has no Transform component then there was a failure in the create entity call
        // and the application of required components
        if (!entityTransform)
        {
            AzToolsFramework::EditorEntityContextRequestBus::Broadcast(&AzToolsFramework::EditorEntityContextRequestBus::Events::DestroyEditorEntity, newEntity->GetId());
            return AZ::EntityId();
        }

        // If supplied set the parent of the new entity
        if (parentId.IsValid())
        {
            entityTransform->SetParent(parentId);
        }

        // Set the new entity's transform to non zero values
        // This helps validate in comparison tests that the transform values of created entities persist during slice operations
        entityTransform->SetLocalUniformScale(5);
        entityTransform->SetLocalRotation(AZ::Vector3RadToDeg(AZ::Vector3(90, 90, 90)));
        entityTransform->SetLocalTranslation(AZ::Vector3(100, 100, 100));

        return entityList.back();
    }

    AZ::SliceComponent::SliceInstanceAddress SliceStabilityTest::InstantiateEditorSlice(AZ::Data::AssetId sliceAssetId, AzToolsFramework::EntityIdList& entityList, const AZ::EntityId& parent /*= AZ::EntityId()*/)
    {
        // Make sure we've created this asset before trying to instantiate it
        auto findIt = m_createdSlices.find(sliceAssetId);
        if (findIt == m_createdSlices.end())
        {
            return AZ::SliceComponent::SliceInstanceAddress();
        }

        // Cache how many instances of this asset exist currently
        size_t currentInstanceCount = findIt->second.size();

        // Acquire the SliceAsset
        AZ::Data::Asset<AZ::SliceAsset> asset = AZ::Data::AssetManager::Instance().FindOrCreateAsset<AZ::SliceAsset>(sliceAssetId, AZ::Data::AssetLoadBehavior::Default);

        if (asset.GetStatus() != AZ::Data::AssetData::AssetStatus::NotLoaded)
        {
            asset.BlockUntilLoadComplete();
        }

        if (!asset)
        {
            return AZ::SliceComponent::SliceInstanceAddress();
        }

        // Instantiate a new slice instance into the editor from the slice asset
        AzToolsFramework::SliceEditorEntityOwnershipServiceRequestBus::BroadcastResult(m_ticket,
            &AzToolsFramework::SliceEditorEntityOwnershipServiceRequestBus::Events::InstantiateEditorSlice,
            asset, AZ::Transform::CreateIdentity());

        // InstantiateEditorSlice queued the actual instantiation logic onto the tick bus queued events
        // Execute the tickbus queue to complete the instantiation
        // This should trigger our OnSliceInstantiated callback
        AZ::TickBus::ExecuteQueuedEvents();

        // Validate that our instances under this asset have grown by 1
        // This confirms that OnSliceInstantiated was called during ExecuteQueuedEvents
        if (findIt->second.size() != (currentInstanceCount + 1))
        {
            return AZ::SliceComponent::SliceInstanceAddress();
        }

        // OnSliceInstantiated has updated the instance list for this asset
        // Acquire it now and check if it's valid
        AZ::SliceComponent::SliceInstanceAddress& newInstanceAddress = findIt->second.back();

        if (!newInstanceAddress.IsValid())
        {
            return AZ::SliceComponent::SliceInstanceAddress();
        }

        // Get the root entity of our new instance and check if it's valid
        AZ::EntityId sliceInstanceRoot;
        AzToolsFramework::ToolsApplicationRequestBus::BroadcastResult(sliceInstanceRoot, &AzToolsFramework::ToolsApplicationRequestBus::Events::GetRootEntityIdOfSliceInstance, newInstanceAddress);

        if (!sliceInstanceRoot.IsValid())
        {
            return AZ::SliceComponent::SliceInstanceAddress();
        }

        // If a parent was provided then make it the parent of our new slice instance
        if (parent.IsValid())
        {
            AZ::TransformBus::Event(sliceInstanceRoot, &AZ::TransformBus::Events::SetParent, parent);
        }

        // Reset our ticket
        m_ticket = AzFramework::SliceInstantiationTicket();

        // For each of the new instances instantiated entities
        // Add them to our live entity id list
        const AZ::SliceComponent::EntityList& instanceEntities = newInstanceAddress.GetInstance()->GetInstantiated()->m_entities;
        for (const AZ::Entity* instanceEntity : instanceEntities)
        {
            if (instanceEntity)
            {
                entityList.emplace_back(instanceEntity->GetId());
            }
        }

        // Return the new instance
        return newInstanceAddress;
    }

    void SliceStabilityTest::ReparentEntity(AZ::EntityId& entity, const AZ::EntityId& newParent)
    {
        AZ::TransformBus::Event(entity, &AZ::TransformBus::Events::SetParent, newParent);
    }

    // A helper to find an entity within an entity list
    // Used to determine whether to update or push an entity to slice
    // As well as to sort our comparison captures in tests
    AZ::Entity* SliceStabilityTest::FindEntityInList(const AZ::EntityId& entityId, const AZ::SliceComponent::EntityList& entityList)
    {
        auto findIt = AZStd::find_if(entityList.begin(), entityList.end(),
            [&entityId](AZ::Entity* entity) -> bool
            {
                if (entity && entity->GetId() == entityId)
                {
                    return true;
                }
                return false;
            });

        if (findIt != entityList.end())
        {
            return *findIt;
        }

        return nullptr;
    }

    // Wrapper around finding an entity in the Editor Root Slice
    AZ::Entity* SliceStabilityTest::FindEntityInEditor(const AZ::EntityId& entityId)
    {
        AZ::SliceComponent* editorRootSlice = nullptr;
        AzToolsFramework::SliceEditorEntityOwnershipServiceRequestBus::BroadcastResult(editorRootSlice,
            &AzToolsFramework::SliceEditorEntityOwnershipServiceRequestBus::Events::GetEditorRootSlice);

        if (!editorRootSlice)
        {
            return nullptr;
        }

        return editorRootSlice->FindEntity(entityId);
    }

    /*
    * EditorEntityContextNotificationBus
    */
    void SliceStabilityTest::OnSliceInstantiated(const AZ::Data::AssetId& sliceAssetId, AZ::SliceComponent::SliceInstanceAddress& sliceAddress, const AzFramework::SliceInstantiationTicket& ticket)
    {
        if (!sliceAssetId.IsValid())
        {
            EXPECT_TRUE(sliceAssetId.IsValid());
            return;
        }

        // We instantiate slices in 2 manners
        // The first is creating a new slice asset and in this case we have no ticket to check against so check the asset id
        // The other is we instantiated an instance from an existing asset and we have a ticket to compare against
        if (ticket == m_ticket || sliceAssetId == m_newSliceId)
        {
            m_createdSlices[sliceAssetId].emplace_back(sliceAddress);

            m_ticket = AzFramework::SliceInstantiationTicket();
        }
    }

    void SliceStabilityTest::OnSliceInstantiationFailed(const AZ::Data::AssetId& sliceAssetId, const AzFramework::SliceInstantiationTicket& ticket)
    {
        // This should never occur for an instantiation we're responsible for
        EXPECT_FALSE(ticket == m_ticket || sliceAssetId == m_newSliceId);
    }

    /*
    * EditorRequestBus
    */
    void SliceStabilityTest::CreateEditorRepresentation(AZ::Entity* entity)
    {
        if (!entity)
        {
            EXPECT_TRUE(entity);
            return;
        }

        // CreateEditorEntity triggers this event so we add required components here
        AzToolsFramework::EditorEntityContextRequestBus::Broadcast(&AzToolsFramework::EditorEntityContextRequestBus::Events::AddRequiredComponents, *entity);
    }

    /*
    * AssetSystemRequestBus
    */
    bool SliceStabilityTest::GetSourceInfoBySourcePath(const char* sourcePath, AZ::Data::AssetInfo& assetInfo, [[maybe_unused]] AZStd::string& watchFolder)
    {
        // Mock stub for GetSourceInfoBySourcePath
        // This call is invoked during Create Slice to predict the asset id of the new slice before it gets processed
        assetInfo.m_relativePath = sourcePath;
        assetInfo.m_assetId = m_newSliceId;

        return true;
    }

    SliceStabilityTest::SliceOperationValidator::SliceOperationValidator() :
        m_serializeContext(nullptr)
    {
    }

    SliceStabilityTest::SliceOperationValidator::~SliceOperationValidator()
    {
        // Destroy any entities within our capture and clear our capture list
        Reset();
    }

    void SliceStabilityTest::SliceOperationValidator::SetSerializeContext(AZ::SerializeContext* serializeContext)
    {
        m_serializeContext = serializeContext;
    }

    bool SliceStabilityTest::SliceOperationValidator::Capture(const AzToolsFramework::EntityIdList& entitiesToCapture)
    {
        // We either haven't released our current capture or were given nothing to capture or we weren't activated
        if (!m_entityStateCapture.empty() || entitiesToCapture.empty() || !m_serializeContext)
        {
            return false;
        }

        // Validate that all entities to capture are real entities in the Editor Entity Context
        // Place their Entity* in a temp list to clone
        AZ::SliceComponent::EntityList captureList;
        for (const AZ::EntityId& entityId : entitiesToCapture)
        {
            AZ::Entity* entity = FindEntityInEditor(entityId);

            if (!entity)
            {
                return false;
            }

            captureList.emplace_back(entity);
        }

        // Clone the entities
        // The clones should not be active within the entity context and are safe from our slice operations
        m_serializeContext->CloneObjectInplace(m_entityStateCapture, &captureList);

        // Success if the clone completed and matches the size of the input
        return m_entityStateCapture.size() == entitiesToCapture.size();
    }

    bool SliceStabilityTest::SliceOperationValidator::Compare(const AZ::SliceComponent::SliceInstanceAddress& instanceToCompare)
    {
        // We've either captured nothing or our instance to compare has no instantiated entities
        if (m_entityStateCapture.empty() || !instanceToCompare.IsValid() || !instanceToCompare.GetInstance()->GetInstantiated())
        {
            return false;
        }

        // Get the instantiated list of entities and early out if the entity count doesn't match out capture
        AZ::SliceComponent::EntityList instanceEntityList = instanceToCompare.GetInstance()->GetInstantiated()->m_entities;
        if (instanceEntityList.size() != m_entityStateCapture.size())
        {
            return false;
        }

        // Since slice instantiation can alter the order of entities against the original input we need to sort our capture to match
        // We do not care if the order of entities is different, only that both sets of entities are identical
        // SortCapture will early out if a comparison entity cannot be found in our capture
        if (!SortCapture(instanceEntityList))
        {
            return false;
        }

        // Build a data patch between our sorted capture and the instantiated comparison entities
        // This will diff every reflected element within both entity lists including: Entity Ids, Parent/Child Hierarchies, Component IDs, Component properties, etc.
        AZ::DataPatch patch;
        bool result = patch.Create(&m_entityStateCapture, &instanceEntityList, AZ::DataPatch::FlagsMap(), AZ::DataPatch::FlagsMap(), m_serializeContext);

        // If the patch has any delta between the two then they do not match
        return result & !patch.IsData();
    }

    bool SliceStabilityTest::SliceOperationValidator::SortCapture(const AzToolsFramework::EntityList& orderToMatch)
    {
        // Since slice instantiation can alter the order of entities against the original input we need to sort our capture to match
        // We do not care if the order of entities is different, only that both sets of entities are identical
        // SortCapture will early out if a comparison entity cannot be found in our capture
        AzToolsFramework::EntityList sortedCapture;
        for (const AZ::Entity* entity : orderToMatch)
        {
            // If an entity is ever nullptr early out
            if (!entity)
            {
                return false;
            }

            // Try and find the entity within our capture state, early out if we can't find it
            AZ::Entity* foundCaptureEntity = FindEntityInList(entity->GetId(), m_entityStateCapture);
            if (!foundCaptureEntity)
            {
                return false;
            }

            // Place the found entity into our temp
            // This builds a sequence of entities that match our orderToMatch list
            sortedCapture.emplace_back(foundCaptureEntity);
        }

        // Update our capture
        m_entityStateCapture = sortedCapture;

        return true;
    }

    void SliceStabilityTest::SliceOperationValidator::Reset()
    {
        // Since our entity capture is made of clones we need to delete them
        for (AZ::Entity* capturedEntity : m_entityStateCapture)
        {
            EXPECT_NE(capturedEntity, nullptr);

            delete capturedEntity;
        }

        m_entityStateCapture.clear();
    }

    void SliceStabilityTest::EntityReferenceComponent::Reflect(AZ::ReflectContext* reflection)
    {
        AZ::SerializeContext* serializeContext = AZ::RttiCast<AZ::SerializeContext*>(reflection);

        if (serializeContext)
        {
            serializeContext->Class<EntityReferenceComponent, AzToolsFramework::Components::EditorComponentBase>()->
                Field("EntityReference", &EntityReferenceComponent::m_entityReference);
        }

    }

    // Sanity check test to confirm validator will catch differences
    TEST_F(SliceStabilityTest, ValidatorCompare_DifferenceInObjects_DifferenceDetected_FT)
    {
        AUTO_RESULT_IF_SETTING_TRUE(UnitTest::prefabSystemSetting, true)

        // Generate a root entity
        AzToolsFramework::EntityIdList liveEntityIds;
        AZ::EntityId rootEntityId = CreateEditorEntity("Root", liveEntityIds);

        ASSERT_TRUE(rootEntityId.IsValid());

        // Capture entity state
        EXPECT_TRUE(m_validator.Capture(liveEntityIds));

        // Create a slice from the root entity
        AZ::SliceComponent::SliceInstanceAddress sliceInstanceAddress;
        AZ::Data::AssetId newSliceAssetId = CreateSlice("NewSlice", liveEntityIds, sliceInstanceAddress);

        ASSERT_TRUE(newSliceAssetId.IsValid());

        // Compare generated slice instance to initial capture state
        EXPECT_TRUE(m_validator.Compare(sliceInstanceAddress));

        // Make a second instance of our new slice
        // This instance should have a unique entity id for its root entity
        AzToolsFramework::EntityIdList newInstanceEntities;
        AZ::SliceComponent::SliceInstanceAddress newInstanceAddress = InstantiateEditorSlice(newSliceAssetId, newInstanceEntities);

        ASSERT_TRUE(newInstanceAddress.IsValid());

        // Validate that our first instance has a single valid entity
        ASSERT_TRUE(sliceInstanceAddress.IsValid());
        ASSERT_TRUE(sliceInstanceAddress.GetInstance()->GetInstantiated());
        ASSERT_EQ(sliceInstanceAddress.GetInstance()->GetInstantiated()->m_entities.size(), 1);
        ASSERT_TRUE(sliceInstanceAddress.GetInstance()->GetInstantiated()->m_entities[0]);

        // Validate that our first instance's entity has rootEntityId as its EntityID
        EXPECT_EQ(sliceInstanceAddress.GetInstance()->GetInstantiated()->m_entities[0]->GetId(), rootEntityId);

        // Validate that our second instance has a single valid entity
        ASSERT_TRUE(newInstanceAddress.IsValid());
        ASSERT_TRUE(newInstanceAddress.GetInstance()->GetInstantiated());
        ASSERT_EQ(newInstanceAddress.GetInstance()->GetInstantiated()->m_entities.size(), 1);
        ASSERT_TRUE(newInstanceAddress.GetInstance()->GetInstantiated()->m_entities[0]);

        // Validate that our two instances have different EntityIDs for their root entities
        EXPECT_NE(sliceInstanceAddress.GetInstance()->GetInstantiated()->m_entities[0]->GetId(), newInstanceAddress.GetInstance()->GetInstantiated()->m_entities[0]->GetId());

        // Compare the new instance against the inital capture
        // We expect the compare to fail since there is a difference in entity ids
        EXPECT_FALSE(m_validator.Compare(newInstanceAddress));
    }
}
