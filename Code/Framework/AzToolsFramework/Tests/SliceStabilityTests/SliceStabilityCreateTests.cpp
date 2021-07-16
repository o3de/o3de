/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Tests/SliceStabilityTests/SliceStabilityTestFramework.h>
#include <AzToolsFramework/ToolsComponents/TransformComponent.h>

namespace UnitTest
{
    TEST_F(SliceStabilityTest, CreateSlice_ValidSingleParentEntityWithValidChildEntity_EntityStateRemainsTheSame_FT)
    {
        AUTO_RESULT_IF_SETTING_TRUE(UnitTest::prefabSystemSetting, true)

        // Generate Parent entity
        AzToolsFramework::EntityIdList liveEntityIds;
        AZ::EntityId parent = CreateEditorEntity("Parent", liveEntityIds);
        ASSERT_TRUE(parent.IsValid());

        // Generate Child entity and set its parent to Parent entity
        ASSERT_TRUE(CreateEditorEntity("Child", liveEntityIds, parent).IsValid());

        // Capture initial hierarchy state
        EXPECT_TRUE(m_validator.Capture(liveEntityIds));

        // Create slice from hierarchy
        AZ::SliceComponent::SliceInstanceAddress sliceInstanceAddress;
        ASSERT_TRUE(CreateSlice("NewSlice", liveEntityIds, sliceInstanceAddress).IsValid());

        // Compare generated slice instance to initial capture state
        EXPECT_TRUE(m_validator.Compare(sliceInstanceAddress));
    }

    TEST_F(SliceStabilityTest, CreateSlice_ValidGrandparentParentChildHierarchy_EntityStateRemainsTheSame_FT)
    {
        AUTO_RESULT_IF_SETTING_TRUE(UnitTest::prefabSystemSetting, true)

        // Build Grandparent->Parent->Child and link parent entities between them
        AzToolsFramework::EntityIdList liveEntityIds;
        AZ::EntityId grandparent = CreateEditorEntity("Grandparent", liveEntityIds);
        ASSERT_TRUE(grandparent.IsValid());

        AZ::EntityId parent = CreateEditorEntity("Parent", liveEntityIds, grandparent);
        ASSERT_TRUE(parent.IsValid());

        ASSERT_TRUE(CreateEditorEntity("Child", liveEntityIds, parent).IsValid());

        // Capture initial hierarchy state
        EXPECT_TRUE(m_validator.Capture(liveEntityIds));

        // Create slice from hierarchy
        AZ::SliceComponent::SliceInstanceAddress sliceInstanceAddress;
        ASSERT_TRUE(CreateSlice("NewSlice", liveEntityIds, sliceInstanceAddress).IsValid());

        // Compare generated slice instance to initial capture state
        EXPECT_TRUE(m_validator.Compare(sliceInstanceAddress));
    }

    TEST_F(SliceStabilityTest, CreateSlice_10DeepParentChildHierarchy_EntityStateRemainsTheSame_FT)
    {
        AUTO_RESULT_IF_SETTING_TRUE(UnitTest::prefabSystemSetting, true)

        AzToolsFramework::EntityIdList liveEntityIds;

        // Build a 10 entity deep hierarchy
        AZ::EntityId parent;
        for (size_t entityCounter = 0; entityCounter < 10; ++entityCounter)
        {
            // For each iteration capture the entity made to be used as the parent for the next
            parent = CreateEditorEntity(AZStd::string::format("Entity Level %zu", entityCounter).c_str(), liveEntityIds, parent);

            ASSERT_TRUE(parent.IsValid());
        }

        // Capture the hierarchy state
        EXPECT_TRUE(m_validator.Capture(liveEntityIds));

        // Create slice from hierarchy
        AZ::SliceComponent::SliceInstanceAddress sliceInstanceAddress;
        ASSERT_TRUE(CreateSlice("NewSlice", liveEntityIds, sliceInstanceAddress).IsValid());

        // Compare generated slice instance to initial capture state
        EXPECT_TRUE(m_validator.Compare(sliceInstanceAddress));
    }

    TEST_F(SliceStabilityTest, CreateSlice_ValidParentWith10ValidChildren_EntityStateRemainsTheSame_FT)
    {
        AUTO_RESULT_IF_SETTING_TRUE(UnitTest::prefabSystemSetting, true)

        AzToolsFramework::EntityIdList liveEntityIds;

        // Create the parent entity and hold on to its id
        AZ::EntityId parent = CreateEditorEntity("Parent", liveEntityIds);
        ASSERT_TRUE(parent.IsValid());

        // Build 10 children and set all of their parent ids to the same parent entity
        for (size_t childEntityCounter = 0; childEntityCounter < 10; ++childEntityCounter)
        {
            ASSERT_TRUE(CreateEditorEntity(AZStd::string::format("Child #%zu", childEntityCounter + 1).c_str(), liveEntityIds, parent).IsValid());
        }

        // Capture the hierarchy state
        EXPECT_TRUE(m_validator.Capture(liveEntityIds));

        // Create slice from hierarchy
        AZ::SliceComponent::SliceInstanceAddress sliceInstanceAddress;
        ASSERT_TRUE(CreateSlice("NewSlice", liveEntityIds, sliceInstanceAddress).IsValid());

        // Compare generated slice instance to initial capture state
        EXPECT_TRUE(m_validator.Compare(sliceInstanceAddress));
    }

    TEST_F(SliceStabilityTest, CreateSlice_ValidParentEntityWithValidChildEntity_OnlyChildEntityAddedToSlice_EntityStateRemainsTheSame_FT)
    {
        AUTO_RESULT_IF_SETTING_TRUE(UnitTest::prefabSystemSetting, true)

        AzToolsFramework::EntityIdList liveEntityIds;

        // Build parent and child entities and connect child to parent
        AZ::EntityId parent = CreateEditorEntity("Parent", liveEntityIds);
        ASSERT_TRUE(parent.IsValid());

        AZ::EntityId child = CreateEditorEntity("Child", liveEntityIds, parent);
        ASSERT_TRUE(child.IsValid());

        // Capture just the child to compare to
        EXPECT_TRUE(m_validator.Capture({ child }));

        // Build a slice from only the child entity
        AZ::SliceComponent::SliceInstanceAddress sliceInstanceAddress;
        ASSERT_TRUE(CreateSlice("NewSlice", { child }, sliceInstanceAddress).IsValid());

        // Validate that the slice instance only contains the child entity
        EXPECT_TRUE(m_validator.Compare(sliceInstanceAddress));
    }

    TEST_F(SliceStabilityTest, CreateSlice_EntityWithExternalReference_ExternalReferenceEntityAutoAddedToSlice_EntityStateRemainsTheSame_FT)
    {
        AUTO_RESULT_IF_SETTING_TRUE(UnitTest::prefabSystemSetting, true)

        // Generate a root entity that will be referenced externally by the entities used to create the slice
        AzToolsFramework::EntityIdList liveEntityIds;
        AZ::EntityId externalRootId = CreateEditorEntity("ExternalRoot", liveEntityIds);
        ASSERT_TRUE(externalRootId.IsValid());

        // Generate the entity that will contain the external entity reference to ExternalRoot and set its parent to ExternalRoot
        AZ::EntityId entityWithExternalReferenceId = CreateEditorEntity("EntityWithExternalReference", liveEntityIds, externalRootId);
        ASSERT_TRUE(entityWithExternalReferenceId.IsValid());

        // Acquire the Entity* of EntityWithExternalReference and validate that we successfully acquired it
        AZ::Entity* entityWithExternalReference = FindEntityInEditor(entityWithExternalReferenceId);
        ASSERT_TRUE(entityWithExternalReference);

        // Deactivate the entity so that we can give it a new component
        entityWithExternalReference->Deactivate();

        // Add an EntityReferenceComponent to EntityWithExternalReference and validate that the component was successfully created
        EntityReferenceComponent* externalEntityReferenceComponent = entityWithExternalReference->CreateComponent<EntityReferenceComponent>();
        ASSERT_TRUE(externalEntityReferenceComponent);

        // Activate the entity
        entityWithExternalReference->Activate();

        // Set its external entity reference field to the ExternalRoot
        externalEntityReferenceComponent->m_entityReference = externalRootId;

        // Capture the hierarchy state
        EXPECT_TRUE(m_validator.Capture(liveEntityIds));

        // Create a slice just from the entity containing the external reference
        // Create slice should detect the external reference and auto add ExternalRoot to the slice
        AZ::SliceComponent::SliceInstanceAddress sliceInstanceAddress;
        ASSERT_TRUE(CreateSlice("Slice1", { entityWithExternalReferenceId }, sliceInstanceAddress).IsValid());

        // Validate that the slice instance contains both entities
        // Confirming that the externally referenced entity was auto added
        EXPECT_TRUE(m_validator.Compare(sliceInstanceAddress));
    }

    TEST_F(SliceStabilityTest, CreateSlice_2ValidWithSharedParent_ParentNotIncludedInSliceCreate_ParentIsGenerated_EntityStateRemainsTheSame_FT)
    {
        AUTO_RESULT_IF_SETTING_TRUE(UnitTest::prefabSystemSetting, true)

        // Create a shared parent that won't be included in the CreateSlice call
        // Including a shared parent will validate that the generated parent becomes a child of the original parent
        // A generated parent is made because CreateSlice will not have a parent entity to work with and one is required
        AzToolsFramework::EntityIdList rootParentEntityId;
        AZ::EntityId rootParentEntity = CreateEditorEntity("RootParentEntity", rootParentEntityId);
        ASSERT_TRUE(rootParentEntity.IsValid());

        // Create two entities and set their parent to rootParentEntity
        AzToolsFramework::EntityIdList liveEntityIds;
        AZ::EntityId entity1Id = CreateEditorEntity("Entity1", liveEntityIds, rootParentEntity);
        ASSERT_TRUE(entity1Id.IsValid());

        AZ::EntityId entity2Id = CreateEditorEntity("Entity2", liveEntityIds, rootParentEntity);
        ASSERT_TRUE(entity2Id.IsValid());

        // Gather the transform data of entity 1 and entity 2
        // Also set the transform data of entity 1 to be different from entity 2
        // Since we're calling multiple TransformBus events on entity 1
        // We can batch them in an Event lambda
        AZ::Transform entity1WorldTransform;
        AZ::TransformBus::Event(entity1Id, [&entity1WorldTransform]
            (AZ::TransformInterface* transformInterface)
            {
                AZ::Vector3 entity1LocalTranslate = transformInterface->GetLocalTranslation();
                AZ::Vector3 entity1LocalRotation = transformInterface->GetLocalRotation();

                transformInterface->SetLocalTranslation(entity1LocalTranslate * 2);
                transformInterface->SetLocalRotation(entity1LocalRotation * 2);

                entity1WorldTransform = transformInterface->GetWorldTM();
            });

        AZ::Transform entity2WorldTransform;
        AZ::TransformBus::EventResult(entity2WorldTransform, entity2Id, &AZ::TransformBus::Events::GetWorldTM);

        // Validate that both transforms are different from the identity
        // Validate that both transforms are different from each other
        EXPECT_FALSE(entity1WorldTransform.IsClose(AZ::Transform::Identity()));
        EXPECT_FALSE(entity2WorldTransform.IsClose(AZ::Transform::Identity()));
        EXPECT_FALSE(entity1WorldTransform.IsClose(entity2WorldTransform));

        // Create a slice from these two entities
        // Create slice should detect that the provided entity list does not contain a shared parent and will generate one
        AZ::SliceComponent::SliceInstanceAddress sliceInstanceAddress;
        ASSERT_TRUE(CreateSlice("NewSlice", liveEntityIds, sliceInstanceAddress).IsValid());

        // Grab the instantiated entities within the generated slice instance
        const AZ::SliceComponent::InstantiatedContainer* sliceInstanceEntities = sliceInstanceAddress.GetInstance()->GetInstantiated();
        ASSERT_TRUE(sliceInstanceEntities);

        // Confirm that it contains 3 entities (Entity1, Entity2, GeneratedRoot)
        EXPECT_EQ(sliceInstanceEntities->m_entities.size(), 3);

        // Validate that the entities are not null
        ASSERT_TRUE(sliceInstanceEntities->m_entities[0]);
        ASSERT_TRUE(sliceInstanceEntities->m_entities[1]);
        ASSERT_TRUE(sliceInstanceEntities->m_entities[2]);

        // Validate that the first two entities have the same ids as Entity1 and Entity2
        EXPECT_EQ(sliceInstanceEntities->m_entities[0]->GetId(), entity1Id);
        EXPECT_EQ(sliceInstanceEntities->m_entities[1]->GetId(), entity2Id);

        // Get Entity1's parent id
        AZ::EntityId entity1Parent;
        AZ::TransformBus::EventResult(entity1Parent, entity1Id, &AZ::TransformBus::Events::GetParentId);

        // Get Entity2's parent id
        AZ::EntityId entity2Parent;
        AZ::TransformBus::EventResult(entity2Parent, entity2Id, &AZ::TransformBus::Events::GetParentId);

        // Confirm the parent id is valid and the same between Entity1 and Entity2
        EXPECT_TRUE(entity1Parent.IsValid());
        EXPECT_EQ(entity1Parent, entity2Parent);

        // Confirm that the parentId is not the original parent but instead a new parent
        EXPECT_NE(entity1Parent, rootParentEntity);

        // Get the parent of entity 1 and 2's parent
        // This should be the original rootParentEntity
        AZ::EntityId grandparent;
        AZ::TransformBus::EventResult(grandparent, entity1Parent, &AZ::TransformBus::Events::GetParentId);

        // Confirm that the new parent is a child of the original parent
        EXPECT_EQ(grandparent, rootParentEntity);

        // Gather the transform information of entity 1 and entity 2 after the create slice operation
        AZ::Transform entity1SliceWorldTransform;
        AZ::TransformBus::EventResult(entity1SliceWorldTransform, entity1Id, &AZ::TransformBus::Events::GetWorldTM);

        AZ::Transform entity2SliceWorldTransform;
        AZ::TransformBus::EventResult(entity2SliceWorldTransform, entity2Id, &AZ::TransformBus::Events::GetWorldTM);

        // Validate that the create slice operation did not impact the transform data
        EXPECT_TRUE(entity1WorldTransform.IsClose(entity1SliceWorldTransform));
        EXPECT_TRUE(entity2WorldTransform.IsClose(entity2SliceWorldTransform));
    }

    TEST_F(SliceStabilityTest, CreateSlice_TestSubsliceOfSameType_EntityStateRemainsTheSame_FT)
    {
        AUTO_RESULT_IF_SETTING_TRUE(UnitTest::prefabSystemSetting, true)

        // Create a Root entity
        AzToolsFramework::EntityIdList liveEntityIds;
        AZ::EntityId rootEntity = CreateEditorEntity("Root", liveEntityIds);
        ASSERT_TRUE(rootEntity.IsValid());

        // Capture entity state
        EXPECT_TRUE(m_validator.Capture(liveEntityIds));

        // Create slice from root entity
        AZ::SliceComponent::SliceInstanceAddress parentSliceInstance;
        AZ::Data::AssetId parentSliceId = CreateSlice("InheritedSlice", liveEntityIds, parentSliceInstance);
        ASSERT_TRUE(parentSliceId.IsValid());

        // Compare generated slice instance to initial capture state
        EXPECT_TRUE(m_validator.Compare(parentSliceInstance));
        m_validator.Reset();

        // Create a second instance of the slice and make it a child of the Root entity
        AZ::SliceComponent::SliceInstanceAddress childSliceInstance = InstantiateEditorSlice(parentSliceId, liveEntityIds, rootEntity);
        ASSERT_TRUE(childSliceInstance.IsValid());

        // Capture this new hierarchy state
        EXPECT_TRUE(m_validator.Capture(liveEntityIds));

        // Create a slice from this new hierarchy
        AZ::SliceComponent::SliceInstanceAddress finalSliceInstance;
        AZ::Data::AssetId finalSliceId = CreateSlice("FinalSlice", liveEntityIds, finalSliceInstance);
        ASSERT_TRUE(finalSliceId.IsValid());

        // Compare genarated slice instance to capture state
        EXPECT_TRUE(m_validator.Compare(finalSliceInstance));
    }

    TEST_F(SliceStabilityTest, CreateSlice_TestSubsliceOfDifferentType_EntityStateRemainsTheSame_FT)
    {
        AUTO_RESULT_IF_SETTING_TRUE(UnitTest::prefabSystemSetting, true)

        // Create a root entity to be used in Slice1
        AzToolsFramework::EntityIdList slice1Entities;
        AZ::EntityId slice1Root = CreateEditorEntity("Slice1Root", slice1Entities);
        ASSERT_TRUE(slice1Root.IsValid());

        // Capture the entity state of Slice1Root
        EXPECT_TRUE(m_validator.Capture(slice1Entities));

        // Create a slice from Slice1Root
        AZ::SliceComponent::SliceInstanceAddress slice1Instance;
        ASSERT_TRUE(CreateSlice("Slice1", slice1Entities, slice1Instance).IsValid());

        // Compare generated slice1Instance to Slice1Root
        EXPECT_TRUE(m_validator.Compare(slice1Instance));
        m_validator.Reset();

        // Create a root entity to be used in Slice2
        AzToolsFramework::EntityIdList slice2Entities;
        AZ::EntityId slice2Root = CreateEditorEntity("Slice2Root", slice2Entities);
        ASSERT_TRUE(slice2Root.IsValid());

        // Capture the entity state of Slice2Root
        EXPECT_TRUE(m_validator.Capture(slice2Entities));

        // Create a slice from Slice2Root
        AZ::SliceComponent::SliceInstanceAddress slice2Instance;
        ASSERT_TRUE(CreateSlice("Slice2", slice2Entities, slice2Instance).IsValid());

        // Compare generated slice2Instance to Slice2Root
        EXPECT_TRUE(m_validator.Compare(slice2Instance));
        m_validator.Reset();

        // Make Slice1Root the parent of Slice2Root
        AZ::TransformBus::Event(slice2Root, &AZ::TransformBus::Events::SetParent, slice1Root);

        // Validate that the parent of Slice2Root was correctly set
        AZ::EntityId slice2RootParent;
        AZ::TransformBus::EventResult(slice2RootParent, slice2Root, &AZ::TransformBus::Events::GetParentId);
        EXPECT_EQ(slice2RootParent, slice1Root);

        // Combine entity lists
        AzToolsFramework::EntityIdList slice3Entities = slice1Entities;
        slice3Entities.insert(slice3Entities.end(), slice2Entities.begin(), slice2Entities.end());

        // Capture final hierarchy state
        EXPECT_TRUE(m_validator.Capture(slice3Entities));

        // Create a slice from final hiararchy
        AZ::SliceComponent::SliceInstanceAddress slice3Instance;
        ASSERT_TRUE(CreateSlice("Slice3", slice3Entities, slice3Instance).IsValid());

        // Compare generated slice instance to capture state
        EXPECT_TRUE(m_validator.Compare(slice3Instance));
    }

    TEST_F(SliceStabilityTest, CreateSlice_Test10DeepSliceAncestry_EntityStateRemainsTheSame_InstanceAncestryIntact_FT)
    {
        AUTO_RESULT_IF_SETTING_TRUE(UnitTest::prefabSystemSetting, true)

        AZ::u32 totalAncestors = 10;

        // Generate a Root entity
        AzToolsFramework::EntityIdList liveEntityIds;
        AZ::EntityId rootEntity = CreateEditorEntity("Root", liveEntityIds);
        ASSERT_TRUE(rootEntity.IsValid());

        // Capture the entity state of Root
        EXPECT_TRUE(m_validator.Capture(liveEntityIds));

        AZ::SliceComponent::SliceInstanceAddress sliceInstanceAddress;
        for (AZ::u32 ancestorCount = 0; ancestorCount < totalAncestors; ++ancestorCount)
        {
            // Continue to make a slice off of Root entity where each iteration Root entity is owned by an instance of the previously made slice
            // For each iteration validate the state of each instance matches the state of the initally captured Root entity state
            ASSERT_TRUE(CreateSlice(AZStd::string::format("Slice Level: %i", ancestorCount + 1).c_str(), liveEntityIds, sliceInstanceAddress).IsValid());

            EXPECT_TRUE(m_validator.Compare(sliceInstanceAddress));
        }

        // Acquire the ancestor hierarchy of Root entity
        // We pass in totalAncestors + 1 for maxLevels to ensure we rule out the ancestry is greater than expected fail state
        AZ::SliceComponent::EntityAncestorList ancestors;
        sliceInstanceAddress.GetReference()->GetInstanceEntityAncestry(rootEntity, ancestors, totalAncestors + 1);

        // Confirm that the ancestor hierarchy size is the same as the number of slices we iteratively built off of Root entity
        EXPECT_EQ(ancestors.size(), totalAncestors);
    }

    TEST_F(SliceStabilityTest, CreateSlice_Test5DeepSliceAncestryWithSubslices_EntityStateRemainsTheSame_FT)
    {
        AUTO_RESULT_IF_SETTING_TRUE(UnitTest::prefabSystemSetting, true)

        // Generate a Root entity
        AzToolsFramework::EntityIdList liveEntityIds;
        AZ::EntityId rootEntity = CreateEditorEntity("Root", liveEntityIds);
        ASSERT_TRUE(rootEntity.IsValid());

        // This loop moves each iteration's hierarchy into a slice instance
        // It then instantiates a second instance and places the second instance under the original hierachy
        // This results in the number of entities growing at a power of 2
        AZ::SliceComponent::SliceInstanceAddress sliceInstanceAddress;
        for (size_t ancestorCount = 0; ancestorCount < 5; ++ancestorCount)
        {
            // Each iteration capture the entity hierarchy state
            EXPECT_TRUE(m_validator.Capture(liveEntityIds));

            // Create a slice from the current hierarchy
            AZ::Data::AssetId newSlice = CreateSlice(AZStd::string::format("Slice Level: %zu", ancestorCount + 1).c_str(), liveEntityIds, sliceInstanceAddress);
            ASSERT_TRUE(newSlice.IsValid());

            // Compare the generated slice instance against the capture state and reset the capture for the next iteration
            EXPECT_TRUE(m_validator.Compare(sliceInstanceAddress));
            m_validator.Reset();

            // Instantiate a second copy of this iteration's slice and set Root entity as its parent
            // liveEntityIds is updated by this call to include the new instances entities
            ASSERT_TRUE(InstantiateEditorSlice(newSlice, liveEntityIds, rootEntity).IsValid());
        }
    }

    TEST_F(SliceStabilityTest, CreateSlice_TestOverride_OverrideAppliesSuccesfully_FT)
    {
        AUTO_RESULT_IF_SETTING_TRUE(UnitTest::prefabSystemSetting, true)

        // Generate a Root entity
        AzToolsFramework::EntityIdList liveEntityIds;
        ASSERT_TRUE(CreateEditorEntity("Root", liveEntityIds).IsValid());

        // Capture the Root entity state
        EXPECT_TRUE(m_validator.Capture(liveEntityIds));

        // Create a slice from Root entity
        AZ::SliceComponent::SliceInstanceAddress sliceInstanceAddress;
        ASSERT_TRUE(CreateSlice("Slice1", liveEntityIds, sliceInstanceAddress).IsValid());

        // Compare the generated slice instance to the capture state and then reset capture state
        EXPECT_TRUE(m_validator.Compare(sliceInstanceAddress));
        m_validator.Reset();
        
        // Validator passing guarantees instance and its instantiated container are not nullptr and instantiated is size 1
        const AZ::SliceComponent::InstantiatedContainer* instantiatedEntities = sliceInstanceAddress.GetInstance()->GetInstantiated();

        // Confirm that the first entity entry is not nullptr
        EXPECT_TRUE(instantiatedEntities->m_entities[0]);

        // Rename the Root entity
        constexpr const char* newRootName = "Renamed Root";
        instantiatedEntities->m_entities[0]->SetName(newRootName);

        // Capture the new entity state which includes the rename
        EXPECT_TRUE(m_validator.Capture(liveEntityIds));

        // Create a slice from the renamed Root
        // This should create a slice with an override on Slice1 that performs the entity rename
        AZ::SliceComponent::SliceInstanceAddress slice2InstanceAddress;
        AZ::Data::AssetId slice2Asset = CreateSlice("Slice2", liveEntityIds, slice2InstanceAddress);
        ASSERT_TRUE(slice2Asset.IsValid());

        // Compare the generated slice instance to the captured entity state
        EXPECT_TRUE(m_validator.Compare(slice2InstanceAddress));

        // Instantiate a second instance of this slice
        // We want to validate that further instantiations after the slice create persist the override
        AzToolsFramework::EntityIdList slice2NewInstanceEntities;
        AZ::SliceComponent::SliceInstanceAddress slice2NewInstanceAddress = InstantiateEditorSlice(slice2Asset, slice2NewInstanceEntities);

        // Confirm the instance is valid
        ASSERT_TRUE(slice2NewInstanceAddress.IsValid());

        // Acquire the instantiated container from the instance and confirm the container is valid
        const AZ::SliceComponent::SliceInstance* slice2NewInstance = slice2NewInstanceAddress.GetInstance();
        const AZ::SliceComponent::InstantiatedContainer* newSlice2InstantiatedEntities = slice2NewInstance->GetInstantiated();

        ASSERT_TRUE(newSlice2InstantiatedEntities);

        // Confirm that the slice instance contains only 1 entity and that its name matches the renamed entity
        ASSERT_EQ(newSlice2InstantiatedEntities->m_entities.size(), 1);
        ASSERT_TRUE(newSlice2InstantiatedEntities->m_entities[0]);
        EXPECT_EQ(newSlice2InstantiatedEntities->m_entities[0]->GetName(), newRootName);
    }
}
