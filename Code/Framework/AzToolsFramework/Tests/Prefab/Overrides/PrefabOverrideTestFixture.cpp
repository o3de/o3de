/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <Prefab/Overrides/PrefabOverrideTestFixture.h>

namespace UnitTest
{
    void PrefabOverrideTestFixture::SetUpEditorFixtureImpl()
    {
        PrefabTestFixture::SetUpEditorFixtureImpl();

        m_prefabOverridePublicInterface = AZ::Interface<PrefabOverridePublicInterface>::Get();
        ASSERT_TRUE(m_prefabOverridePublicInterface);
    }

    AZStd::pair<AZ::EntityId, AZ::EntityId> PrefabOverrideTestFixture::CreateEntityInNestedPrefab()
    {
        AZ::EntityId entityToBePutUnderPrefabId = CreateEntityUnderRootPrefab("EntityUnderPrefab");

        // Rather than hardcode a path, use a path from settings registry since that will work on all platforms.
        AZ::SettingsRegistryInterface* registry = AZ::SettingsRegistry::Get();
        AZ::IO::FixedMaxPath path;
        registry->Get(path.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_EngineRootFolder);

        AZ::EntityId nestedPrefabContainerId = CreatePrefab(AzToolsFramework::EntityIdList{ entityToBePutUnderPrefabId }, path);

        // Append '1' to the path so that there is no path collision when creating another prefab.
        AZ::EntityId prefabContainerId = CreatePrefab(AzToolsFramework::EntityIdList{ nestedPrefabContainerId }, path.Append("1"));

        AZ::EntityId nestedPrefabEntityId;
        InstanceOptionalReference prefabInstance = m_instanceEntityMapperInterface->FindOwningInstance(prefabContainerId);
        EXPECT_TRUE(prefabInstance.has_value());

        // Fetch the id of the entity within the nested prefab as it changes after putting it in a prefab.
        prefabInstance->get().GetNestedInstances(
            [&nestedPrefabEntityId](AZStd::unique_ptr<Instance>& nestedInstance)
            {
                nestedInstance->GetEntities(
                    [&nestedPrefabEntityId](const AZStd::unique_ptr<AZ::Entity>& entity)
                    {
                        nestedPrefabEntityId = entity->GetId();
                        return true;
                    });
            });

        return AZStd::pair(nestedPrefabEntityId, prefabContainerId);
    }

    void PrefabOverrideTestFixture::CreateAndValidateOverride(AZ::EntityId entityId)
    {
        // Validate that there are no override present upon prefab creation.
        ASSERT_FALSE(m_prefabOverridePublicInterface->AreOverridesPresent(entityId));

        // Modify the transform component. Since the focus will be on the root prefab by default, this becomes an override.
        AZ::TransformBus::Event(entityId, &AZ::TransformInterface::SetWorldX, 10.0f);
        m_prefabPublicInterface->GenerateUndoNodesForEntityChangeAndUpdateCache(entityId, m_undoStack->GetTop());

        // Validate that override is present on the entity.
        EXPECT_TRUE(m_prefabOverridePublicInterface->AreOverridesPresent(entityId));
    }

} // namespace UnitTest
