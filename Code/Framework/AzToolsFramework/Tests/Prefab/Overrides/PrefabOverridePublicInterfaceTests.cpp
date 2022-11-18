/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/ToolsComponents/TransformComponent.h>
#include <Prefab/Overrides/PrefabOverrideTestFixture.h>

namespace UnitTest
{
    using PrefabOverridePublicInterfaceTest = PrefabOverrideTestFixture;

    TEST_F(PrefabOverridePublicInterfaceTest, AreOverridesPresentWorksWithOverrideFromImmediateParent)
    {
        AZ::EntityId newEntityId, parentContainerId, grandparentContainerId;
        CreateEntityInNestedPrefab(newEntityId, parentContainerId, grandparentContainerId);
        
        CreateAndValidateEditEntityOverride(newEntityId, grandparentContainerId);
    }

    TEST_F(PrefabOverridePublicInterfaceTest, AreOverridesPresentWorksWithOverrideFromLevel)
    {
        AZ::EntityId newEntityId, parentContainerId, grandparentContainerId;
        CreateEntityInNestedPrefab(newEntityId, parentContainerId, grandparentContainerId);
        AZ::EntityId levelContainerId = m_prefabEditorEntityOwnershipInterface->GetRootPrefabInstance()->get().GetContainerEntityId();
        CreateAndValidateEditEntityOverride(newEntityId, levelContainerId);
    }

    TEST_F(PrefabOverridePublicInterfaceTest, AreOverridesPresentReturnsFalseWhenNoOverride)
    {
        AZ::EntityId newEntityId, parentContainerId, grandparentContainerId;
        CreateEntityInNestedPrefab(newEntityId, parentContainerId, grandparentContainerId);

        EditEntityAndValidateNoOverride(newEntityId);
    }

    TEST_F(PrefabOverridePublicInterfaceTest, RevertOverridesOnEntityWithOverrides)
    {
        AZ::EntityId newEntityId, parentContainerId, grandparentContainerId;
        CreateEntityInNestedPrefab(newEntityId, parentContainerId, grandparentContainerId);

        m_prefabFocusPublicInterface->FocusOnOwningPrefab(grandparentContainerId);

        // Modify the transform component.
        AZ::TransformBus::Event(newEntityId, &AZ::TransformInterface::SetWorldX, 10.0f);
        m_prefabPublicInterface->GenerateUndoNodesForEntityChangeAndUpdateCache(newEntityId, m_undoStack->GetTop());
        PropagateAllTemplateChanges();

        // Validate that overrides are present on the entity.
        ASSERT_TRUE(m_prefabOverridePublicInterface->AreOverridesPresent(newEntityId));

        // Revert the overrides on the entity.
        EXPECT_TRUE(m_prefabOverridePublicInterface->RevertOverrides(newEntityId));
        PropagateAllTemplateChanges();

        // Validate that overrides are absent upon reverting.
        EXPECT_FALSE(m_prefabOverridePublicInterface->AreOverridesPresent(newEntityId));
        float worldX;
        AZ::TransformBus::EventResult(worldX, newEntityId, &AZ::TransformInterface::GetWorldX);
        EXPECT_EQ(worldX, 0.0f);

        Undo();
        EXPECT_TRUE(m_prefabOverridePublicInterface->AreOverridesPresent(newEntityId));
        AZ::TransformBus::EventResult(worldX, newEntityId, &AZ::TransformInterface::GetWorldX);
        EXPECT_EQ(worldX, 10.0f);

        Redo();
        EXPECT_FALSE(m_prefabOverridePublicInterface->AreOverridesPresent(newEntityId));
        AZ::TransformBus::EventResult(worldX, newEntityId, &AZ::TransformInterface::GetWorldX);
        EXPECT_EQ(worldX, 0.0f);
    }

    TEST_F(PrefabOverridePublicInterfaceTest, RevertOverridesOnEntityWithoutOverrides)
    {
        AZ::EntityId newEntityId, parentContainerId, grandparentContainerId;
        CreateEntityInNestedPrefab(newEntityId, parentContainerId, grandparentContainerId);

        m_prefabFocusPublicInterface->FocusOnOwningPrefab(grandparentContainerId);

        // Validate that overrides are present on the entity.
        ASSERT_FALSE(m_prefabOverridePublicInterface->AreOverridesPresent(newEntityId));

        // RevertOverrides should return false since there are no overrides on the entity.
        EXPECT_FALSE(m_prefabOverridePublicInterface->RevertOverrides(newEntityId));
    }
} // namespace UnitTest
