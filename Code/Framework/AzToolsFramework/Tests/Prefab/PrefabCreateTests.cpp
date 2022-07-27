/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/Entity/EditorEntityHelpers.h>
#include <Prefab/PrefabTestDomUtils.h>
#include <Prefab/PrefabTestFixture.h>

namespace UnitTest
{
    using PrefabCreateTest = PrefabTestFixture;

    TEST_F(PrefabCreateTest, CreateEntity_CreateEntitySucceeds)
    {
        PrefabEntityResult createEntityResult = m_prefabPublicInterface->CreateEntity(AZ::EntityId(), AZ::Vector3());

        // Verify that a valid entity is created.
        AZ::EntityId createdEntityId = createEntityResult.GetValue();
        EXPECT_TRUE(createdEntityId.IsValid());
        AZ::Entity* createdEntity = AzToolsFramework::GetEntityById(createdEntityId);
        EXPECT_TRUE(createdEntity != nullptr);
    }

    TEST_F(PrefabCreateTest, CreateEntity_PreemptiveRefreshOnCachedInstanceDom)
    {
        PrefabEntityResult createEntityResult = m_prefabPublicInterface->CreateEntity(AZ::EntityId(), AZ::Vector3());

        // Verify that a valid entity is created.
        AZ::EntityId createdEntityId = createEntityResult.GetValue();
        ASSERT_TRUE(createdEntityId.IsValid());
        AZ::Entity* createdEntity = AzToolsFramework::GetEntityById(createdEntityId);
        ASSERT_TRUE(createdEntity != nullptr);

        // Verify that cached instance DOM matches template DOM due to preemptive cache update.
        InstanceOptionalReference owningInstance = m_instanceEntityMapperInterface->FindOwningInstance(createdEntityId);
        ASSERT_TRUE(owningInstance.has_value());
        ValidateCachedInstanceDomMatchesTemplateDom(owningInstance->get());
    }
} // namespace UnitTest
