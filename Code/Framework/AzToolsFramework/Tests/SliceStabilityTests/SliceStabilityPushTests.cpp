/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Tests/SliceStabilityTests/SliceStabilityTestFramework.h>

namespace UnitTest
{
    TEST_F(SliceStabilityTest, PushToSlice_PushSingleEntityToSlice_EntityStateRemainsTheSame_FT)
    {
        AUTO_RESULT_IF_SETTING_TRUE(UnitTest::prefabSystemSetting, true)

        // Create an entity to be used in a slice
        AzToolsFramework::EntityIdList liveEntityIds;
        AZ::EntityId sliceEntity = CreateEditorEntity("SliceEntity", liveEntityIds);
        ASSERT_TRUE(sliceEntity.IsValid());

        // Capture current entity state
        EXPECT_TRUE(m_validator.Capture(liveEntityIds));

        // Create a slice from the entity
        AZ::SliceComponent::SliceInstanceAddress sliceInstanceAddress;
        ASSERT_TRUE(CreateSlice("NewSlice", liveEntityIds, sliceInstanceAddress).IsValid());

        // Compare the generated slice instance against the captured entity state
        EXPECT_TRUE(m_validator.Compare(sliceInstanceAddress));
        m_validator.Reset();

        // Create an entity to be pushed to slice and set its parent to be the first SliceEntity
        AZ::EntityId addedEntity = CreateEditorEntity("AddedEntity", liveEntityIds, sliceEntity);
        ASSERT_TRUE(addedEntity.IsValid());

        // Capture current entity state
        EXPECT_TRUE(m_validator.Capture(liveEntityIds));

        // Push AddedEntity to the existing slice instance
        ASSERT_TRUE(PushEntitiesToSlice(sliceInstanceAddress, liveEntityIds));

        // Compare the updated slice instance against the captured entity state
        EXPECT_TRUE(m_validator.Compare(sliceInstanceAddress));
    }

    TEST_F(SliceStabilityTest, PushToSlice_PushSingleParentEntityWithChildEntity_EntityStateRemainsTheSame_FT)
    {
        AUTO_RESULT_IF_SETTING_TRUE(UnitTest::prefabSystemSetting, true)

        // Create an entity to be used as a slice root
        AzToolsFramework::EntityIdList liveEntityIds;
        AZ::EntityId sliceEntity = CreateEditorEntity("SliceEntity", liveEntityIds);
        ASSERT_TRUE(sliceEntity.IsValid());

        // Capture current entity state
        EXPECT_TRUE(m_validator.Capture(liveEntityIds));

        // Create a slice from the current entity state
        AZ::SliceComponent::SliceInstanceAddress sliceInstanceAddress;
        ASSERT_TRUE(CreateSlice("NewSlice", liveEntityIds, sliceInstanceAddress).IsValid());

        // Compare the generated slice instance against the captured entity state
        EXPECT_TRUE(m_validator.Compare(sliceInstanceAddress));
        m_validator.Reset();

        // Create a parent and child entity to be pushed to the slice
        // Set AddedParent's parent to be SliceEntity
        // Set AddedChild's parent to be AddedParent
        AZ::EntityId addedParent = CreateEditorEntity("AddedParent", liveEntityIds, sliceEntity);
        ASSERT_TRUE(addedParent.IsValid());

        AZ::EntityId addedChild = CreateEditorEntity("AddedChild", liveEntityIds, addedParent);
        ASSERT_TRUE(addedChild.IsValid());

        // Capture the current entity state
        EXPECT_TRUE(m_validator.Capture(liveEntityIds));

        // Push AddedParent and AddedChild to the existing slice instance
        ASSERT_TRUE(PushEntitiesToSlice(sliceInstanceAddress, liveEntityIds));

        // Compare the updated slice instance against the captured entity state
        EXPECT_TRUE(m_validator.Compare(sliceInstanceAddress));
    }

    // Disabled in SPEC-3077
    TEST_F(SliceStabilityTest, DISABLED_PushToSlice_PushGrandparentParentChildHierarchy_EntityStateRemainsTheSame_FT)
    {
        AUTO_RESULT_IF_SETTING_TRUE(UnitTest::prefabSystemSetting, true)

        // Create an entity to be used as a slice root
        AzToolsFramework::EntityIdList liveEntityIds;
        AZ::EntityId sliceEntity = CreateEditorEntity("SliceEntity", liveEntityIds);
        ASSERT_TRUE(sliceEntity.IsValid());

        // Capture current entity state
        EXPECT_TRUE(m_validator.Capture(liveEntityIds));

        // Create a slice from current entity state
        AZ::SliceComponent::SliceInstanceAddress sliceInstanceAddress;
        ASSERT_TRUE(CreateSlice("NewSlice", liveEntityIds, sliceInstanceAddress).IsValid());

        // Compare the generated slice instance against the captured entity state
        EXPECT_TRUE(m_validator.Compare(sliceInstanceAddress));
        m_validator.Reset();

        // Create a grandparent->parent->child to be pushed to the slice and connect their parent hierarchy accordingly
        AZ::EntityId addedGrandparent = CreateEditorEntity("AddedGrandParent", liveEntityIds, sliceEntity);
        ASSERT_TRUE(addedGrandparent.IsValid());

        AZ::EntityId addedParent = CreateEditorEntity("AddedParent", liveEntityIds, addedGrandparent);
        ASSERT_TRUE(addedParent.IsValid());

        AZ::EntityId addedChild = CreateEditorEntity("AddedChild", liveEntityIds, addedParent);
        ASSERT_TRUE(addedChild.IsValid());

        // Capture current entity state
        EXPECT_TRUE(m_validator.Capture(liveEntityIds));

        // Push grandparent, parent, and child to slice
        ASSERT_TRUE(PushEntitiesToSlice(sliceInstanceAddress, liveEntityIds));

        // Compare the updated slice instance against the captured entity state
        EXPECT_TRUE(m_validator.Compare(sliceInstanceAddress));
    }

    TEST_F(SliceStabilityTest, PushToSlice_Push10DeepParentChildHierarchy_EntityStateRemainsTheSame_FT)
    {
        AUTO_RESULT_IF_SETTING_TRUE(UnitTest::prefabSystemSetting, true)

        // Create an entity to be used as a slice root
        AzToolsFramework::EntityIdList liveEntityIds;
        AZ::EntityId sliceEntity = CreateEditorEntity("SliceEntity", liveEntityIds);
        ASSERT_TRUE(sliceEntity.IsValid());

        // Capture current entity state
        EXPECT_TRUE(m_validator.Capture(liveEntityIds));

        // Create a slice from current entity state
        AZ::SliceComponent::SliceInstanceAddress sliceInstanceAddress;
        ASSERT_TRUE(CreateSlice("NewSlice", liveEntityIds, sliceInstanceAddress).IsValid());

        // Compare the generated slice instance against the captured entity state
        EXPECT_TRUE(m_validator.Compare(sliceInstanceAddress));
        m_validator.Reset();

        // Generate 10 new entities and set each entity's parent to be the entity generated before them
        // This creates a 10 child deep hierarchy that we will push to slice
        AZ::EntityId parent = sliceEntity;
        for (size_t entityCounter = 0; entityCounter < 10; ++entityCounter)
        {
            parent = CreateEditorEntity(AZStd::string::format("Added Entity Level %zu", entityCounter).c_str(), liveEntityIds, parent);
            ASSERT_TRUE(parent.IsValid());
        }

        // Capture the current entity state
        EXPECT_TRUE(m_validator.Capture(liveEntityIds));

        // Push the newly created entities into the existing slice
        ASSERT_TRUE(PushEntitiesToSlice(sliceInstanceAddress, liveEntityIds));

        // Compare the updated slice instance against the captured entity state
        EXPECT_TRUE(m_validator.Compare(sliceInstanceAddress));
    }

    TEST_F(SliceStabilityTest, PushToSlice_Push10Children_EntityStateRemainsTheSame_FT)
    {
        AUTO_RESULT_IF_SETTING_TRUE(UnitTest::prefabSystemSetting, true)

        // Create an entity to be used as a slice root
        AzToolsFramework::EntityIdList liveEntityIds;
        AZ::EntityId sliceEntity = CreateEditorEntity("SliceEntity", liveEntityIds);
        ASSERT_TRUE(sliceEntity.IsValid());

        // Capture current entity state
        EXPECT_TRUE(m_validator.Capture(liveEntityIds));

        // Create a slice from current entity state
        AZ::SliceComponent::SliceInstanceAddress sliceInstanceAddress;
        ASSERT_TRUE(CreateSlice("NewSlice", liveEntityIds, sliceInstanceAddress).IsValid());

        // Compare the generated slice instance against the captured entity state
        EXPECT_TRUE(m_validator.Compare(sliceInstanceAddress));
        m_validator.Reset();

        AZ::EntityId addedEntity;
        AzToolsFramework::EntityIdList entitiesToPush;
        for (size_t childEntityCounter = 0; childEntityCounter < 10; ++childEntityCounter)
        {
            // Generate a set of children who share the same parent (SliceEntity) and add them to the list of entities to push
            addedEntity = CreateEditorEntity(AZStd::string::format("Child #%zu", childEntityCounter).c_str(), liveEntityIds, sliceEntity);
            ASSERT_TRUE(addedEntity.IsValid());

            entitiesToPush.emplace_back(addedEntity);
        }

        // Capture current entity state
        EXPECT_TRUE(m_validator.Capture(liveEntityIds));

        // Push the created child entities to the existing slice
        ASSERT_TRUE(PushEntitiesToSlice(sliceInstanceAddress, liveEntityIds));

        // Compare the updated slice instance against the captured entity state
        EXPECT_TRUE(m_validator.Compare(sliceInstanceAddress));
    }

    TEST_F(SliceStabilityTest, PushToSlice_PushNestedSliceOfDifferentType_EntityStateRemainsTheSame_FT)
    {
        AUTO_RESULT_IF_SETTING_TRUE(UnitTest::prefabSystemSetting, true)

        // Create an entity to be used for Slice1's root
        AzToolsFramework::EntityIdList slice1Entities;
        AZ::EntityId slice1Root = CreateEditorEntity("slice1Root", slice1Entities);
        ASSERT_TRUE(slice1Root.IsValid());

        // Capture entity state for slice1Root
        EXPECT_TRUE(m_validator.Capture(slice1Entities));

        // Create a slice from slice1Root
        AZ::SliceComponent::SliceInstanceAddress slice1Instance;
        ASSERT_TRUE(CreateSlice("Slice1", slice1Entities, slice1Instance).IsValid());

        // Compare the state of slice1Instance to the captured state of slice1Root
        EXPECT_TRUE(m_validator.Compare(slice1Instance));
        m_validator.Reset();

        // Create an entity to be used for Slice2's root and make its parent slice1Root
        AzToolsFramework::EntityIdList slice2Entities;
        AZ::EntityId slice2Root = CreateEditorEntity("Slice2Root", slice2Entities);
        ASSERT_TRUE(slice2Root.IsValid());

        // Provide Slice2Root a child entity to confirm all entities in Slice2 are included in the push
        ASSERT_TRUE(CreateEditorEntity("Slice2Child", slice2Entities, slice2Root).IsValid());

        // Capture entity state for Slice2Root
        EXPECT_TRUE(m_validator.Capture(slice2Entities));

        // Create a slice from slice2Root
        AZ::SliceComponent::SliceInstanceAddress slice2Instance;
        ASSERT_TRUE(CreateSlice("Slice2", slice2Entities, slice2Instance).IsValid());

        // Compare the state of slice2Instance to the captured state of slice2Root
        EXPECT_TRUE(m_validator.Compare(slice2Instance));
        m_validator.Reset();

        // Parent slice2Root under slice1Root to prepare for the push
        ReparentEntity(slice2Root, slice1Root);

        // Combine the current entity lists
        AzToolsFramework::EntityIdList totalEntities = slice1Entities;
        totalEntities.insert(totalEntities.end(), slice2Entities.begin(), slice2Entities.end());

        // Capture the total entity hierarchy state
        EXPECT_TRUE(m_validator.Capture(totalEntities));

        // Push the slice2Root entity into slice1Instance
        ASSERT_TRUE(PushEntitiesToSlice(slice1Instance, totalEntities));

        // Compare the updated slice instance against the captured entity state
        EXPECT_TRUE(m_validator.Compare(slice1Instance));
    }

    TEST_F(SliceStabilityTest, PushToSliceAndCreateSlice_ValidateCombinationOfPushCreateOperations_EntityStateRemainsTheSame_FT)
    {
        AUTO_RESULT_IF_SETTING_TRUE(UnitTest::prefabSystemSetting, true)

        // Create Slice1 root
        AzToolsFramework::EntityIdList slice1Entities;
        AZ::EntityId slice1Root = CreateEditorEntity("Slice1Root", slice1Entities);
        ASSERT_TRUE(slice1Root.IsValid());

        EXPECT_TRUE(m_validator.Capture(slice1Entities));

        // Create Slice1 from Slice1 root
        AZ::SliceComponent::SliceInstanceAddress slice1Instance;
        AZ::Data::AssetId slice1Asset = CreateSlice("Slice1", slice1Entities, slice1Instance);
        ASSERT_TRUE(slice1Asset.IsValid());

        // Validate that Slice1 instance did not change the structure of Slice1 root
        EXPECT_TRUE(m_validator.Compare(slice1Instance));
        m_validator.Reset();

        // Create Slice1 child and make Slice1 root its parent
        AZ::EntityId slice1Child = CreateEditorEntity("Slice1Child", slice1Entities, slice1Root);
        ASSERT_TRUE(slice1Child.IsValid());

        EXPECT_TRUE(m_validator.Capture(slice1Entities));

        // Push Slice1 child to Slice1
        ASSERT_TRUE(PushEntitiesToSlice(slice1Instance, slice1Entities));

        // Validate that Slice1 root and child did not change during push
        EXPECT_TRUE(m_validator.Compare(slice1Instance));
        m_validator.Reset();

        // Instantiate a second instance of Slice1 and make the original Slice 1 child its parent
        AzToolsFramework::EntityIdList secondSlice1InstanceEntities;
        ASSERT_TRUE(InstantiateEditorSlice(slice1Asset, secondSlice1InstanceEntities, slice1Child).IsValid());

        // Slice 2 entities will be the combination of both Slice1 instances
        AzToolsFramework::EntityIdList slice2Entities = slice1Entities;
        slice2Entities.insert(slice2Entities.end(), secondSlice1InstanceEntities.begin(), secondSlice1InstanceEntities.end());

        EXPECT_TRUE(m_validator.Capture(slice2Entities));

        // Create slice 2
        AZ::SliceComponent::SliceInstanceAddress slice2Instance;
        ASSERT_TRUE(CreateSlice("Slice2", slice2Entities, slice2Instance).IsValid());

        // Validate that entities in the Slice 2 instance are structurally the same as the input entities in its creation
        EXPECT_TRUE(m_validator.Compare(slice2Instance));
    }
}
