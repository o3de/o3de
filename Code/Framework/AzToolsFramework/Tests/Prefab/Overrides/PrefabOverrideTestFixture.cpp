/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Prefab/Overrides/PrefabOverrideTestFixture.h>

namespace UnitTest
{
    void PrefabOverrideTestFixture::SetUpEditorFixtureImpl()
    {
        PrefabTestFixture::SetUpEditorFixtureImpl();

        m_prefabOverridePublicInterface = AZ::Interface<PrefabOverridePublicInterface>::Get();
        ASSERT_TRUE(m_prefabOverridePublicInterface);

        m_prefabFocusPublicInterface = AZ::Interface<PrefabFocusPublicInterface>::Get();
        ASSERT_TRUE(m_prefabFocusPublicInterface);
    }

    void PrefabOverrideTestFixture::CreateEntityInNestedPrefab(
        AZ::EntityId& newEntityId, AZ::EntityId& parentContainerId, AZ::EntityId& grandparentContainerId)
    {
        AZ::EntityId entityUnderRootId = CreateEditorEntityUnderRoot("EntityUnderPrefab");

        AZ::IO::Path path;
        m_settingsRegistryInterface->Get(path.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_EngineRootFolder);

        AZ::EntityId nestedPrefabContainerId = CreateEditorPrefab(path, AzToolsFramework::EntityIdList{ entityUnderRootId });

        // Append '1' to the path so that there is no path collision when creating another prefab.
        grandparentContainerId = CreateEditorPrefab(path.Append("1"), AzToolsFramework::EntityIdList{ nestedPrefabContainerId });

        PropagateAllTemplateChanges();

        InstanceOptionalReference prefabInstance = m_instanceEntityMapperInterface->FindOwningInstance(grandparentContainerId);
        EXPECT_TRUE(prefabInstance.has_value());

        // Fetch the id of the entity within the nested prefab as it changes after putting it in a prefab.
        prefabInstance->get().GetNestedInstances(
            [&newEntityId, &parentContainerId](AZStd::unique_ptr<Instance>& nestedInstance)
            {
                nestedInstance->GetEntities(
                    [&newEntityId](const AZStd::unique_ptr<AZ::Entity>& entity)
                    {
                        newEntityId = entity->GetId();
                        return true;
                    });
                parentContainerId = nestedInstance->GetContainerEntityId();
            });
    }

    void PrefabOverrideTestFixture::CreateAndValidateEditEntityOverride(AZ::EntityId entityId, AZ::EntityId ancestorEntityId)
    {
        m_prefabFocusPublicInterface->FocusOnOwningPrefab(ancestorEntityId);

        // Validate that there are no overrides present on the entity.
        ASSERT_FALSE(m_prefabOverridePublicInterface->AreOverridesPresent(entityId));

        // Modify the transform component.
        AZ::TransformBus::Event(entityId, &AZ::TransformInterface::SetWorldX, 10.0f);
        m_prefabPublicInterface->GenerateUndoNodesForEntityChangeAndUpdateCache(entityId, m_undoStack->GetTop());

        // Validate that override is present on the entity.
        EXPECT_TRUE(m_prefabOverridePublicInterface->AreOverridesPresent(entityId));
    }

    void PrefabOverrideTestFixture::EditEntityAndValidateNoOverride(AZ::EntityId entityId)
    {
        m_prefabFocusPublicInterface->FocusOnOwningPrefab(entityId);

        // Validate that there are no overrides present on the entity.
        ASSERT_FALSE(m_prefabOverridePublicInterface->AreOverridesPresent(entityId));

        // Modify the transform component.
        AZ::TransformBus::Event(entityId, &AZ::TransformInterface::SetWorldX, 10.0f);
        m_prefabPublicInterface->GenerateUndoNodesForEntityChangeAndUpdateCache(entityId, m_undoStack->GetTop());

        // Validate that overrides are still not present on the entity since the edit went to the template DOM directly.
        EXPECT_FALSE(m_prefabOverridePublicInterface->AreOverridesPresent(entityId));
    }

} // namespace UnitTest
