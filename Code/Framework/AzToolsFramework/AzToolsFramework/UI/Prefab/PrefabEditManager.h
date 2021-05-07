/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/EntityId.h>
#include <AzCore/Memory/SystemAllocator.h>

#include <AzToolsFramework/Prefab/PrefabPublicInterface.h>
#include <AzToolsFramework/UI/Prefab/PrefabEditInterface.h>
#include <AzToolsFramework/UI/Prefab/PrefabEditPublicInterface.h>
#include <AzToolsFramework/ViewportSelection/EditorInteractionInterface.h>

namespace AzToolsFramework
{
    namespace Prefab
    {
        class PrefabEditManager final
            : private PrefabEditInterface
            , private PrefabEditPublicInterface
            , private EditorInteractionInterface
        {
        public:
            AZ_CLASS_ALLOCATOR(PrefabEditManager, AZ::SystemAllocator, 0);

            PrefabEditManager();
            ~PrefabEditManager();

        private:
            // PrefabEditInterface...
            void EditPrefab(AZ::EntityId entityId) override;

            // PrefabEditPublicInterface...
            void EditOwningPrefab(AZ::EntityId entityId) override;
            bool IsOwningPrefabBeingEdited(AZ::EntityId entityId) override;
            bool IsOwningPrefabInEditStack(AZ::EntityId entityId) override;

            // EditorInteractionInterface...
            AZ::EntityId RedirectEntitySelection(AZ::EntityId entityId) override;

            // EntityId of the container for the prefab instance that is currently being edited
            AZ::EntityId m_editedPrefabContainerId;
            // Store the container ids for all prefab instances in the hierarchy that is being edited for quick reference
            AZStd::set<AZ::EntityId> m_editedPrefabHierarchyCache;

            PrefabPublicInterface* m_prefabPublicInterface;
        };
    }
}
