/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/Prefab/PrefabFocusPublicInterface.h>
#include <AzToolsFramework/Prefab/Overrides/PrefabOverridePublicInterface.h>
#include <Prefab/PrefabTestFixture.h>

namespace UnitTest
{
    using namespace AzToolsFramework::Prefab;

    class PrefabOverrideTestFixture : public PrefabTestFixture
    {
    protected:
        void SetUpEditorFixtureImpl() override;

        //! Creates an entity within a nested prefab under level. The setup looks like:
        //! | Level
        //!   | Prefab (grandparentContainerId)
        //!     | Nested prefab (parentContainerId)
        //!       | New Entity
        //! @param newEntityId The new entity id created under nested prefab.
        //! @param parentContainerId The container entity id of the nested prefab.
        //! @param grandparentContainerId The container entity id of the top-level prefab.
        void CreateEntityInNestedPrefab(AZ::EntityId& newEntityId, AZ::EntityId& parentContainerId, AZ::EntityId& grandparentContainerId);

        //! Focuses on the owning instance of the ancestor entity id and modifies the entity matching the entity id.
        //! This will make the edit become an override.
        //! @param entityId The id of entity to modify.
        //! @param ancestorEntityId The id of an ancestor entity to use for focusing on its owning prefab.
        void CreateAndValidateEditEntityOverride(AZ::EntityId entityId, AZ::EntityId ancestorEntityId);

        //! Focuses on the owning instance of the entity id and modifies it, which makes this a template edit.
        //! @param entityId The Id of the entity to modify.
        void EditEntityAndValidateNoOverride(AZ::EntityId entityId);

        PrefabOverridePublicInterface* m_prefabOverridePublicInterface = nullptr;
        PrefabFocusPublicInterface* m_prefabFocusPublicInterface = nullptr;
    };
} // namespace UnitTest
