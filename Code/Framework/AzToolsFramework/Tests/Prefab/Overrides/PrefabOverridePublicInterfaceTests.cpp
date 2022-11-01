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
} // namespace UnitTest
