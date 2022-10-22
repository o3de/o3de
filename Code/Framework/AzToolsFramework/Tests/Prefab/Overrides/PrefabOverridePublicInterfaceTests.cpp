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

    TEST_F(PrefabOverridePublicInterfaceTest, AreOverridesPresentWorksWithParentOverride)
    {
        AZStd::pair<AZ::EntityId, AZ::EntityId> nestedEntityParentPrefabPair = CreateEntityInNestedPrefab();

        auto* prefabFocusPublicInterface = AZ::Interface<PrefabFocusPublicInterface>::Get();
        ASSERT_TRUE(prefabFocusPublicInterface != nullptr);
        prefabFocusPublicInterface->FocusOnOwningPrefab(nestedEntityParentPrefabPair.second);
        
        CreateAndValidateOverride(nestedEntityParentPrefabPair.first);
    }

    TEST_F(PrefabOverridePublicInterfaceTest, AreOverridesPresentWorksWithGrandParentOverride)
    {
        // By default, the focus exists on the level prefab. So, we don't need to explicitly set focus here.
        AZStd::pair<AZ::EntityId, AZ::EntityId> nestedEntityParentPrefabPair = CreateEntityInNestedPrefab();
        CreateAndValidateOverride(nestedEntityParentPrefabPair.first);
    }
} // namespace UnitTest
