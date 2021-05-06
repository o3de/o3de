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
#include <AzToolsFramework/ViewportSelection/EditorInteractionInterface.h>

namespace AzToolsFramework
{
    namespace Prefab
    {
        class PrefabEditManager final
            : private PrefabEditInterface
            , private EditorInteractionInterface
        {
        public:
            AZ_CLASS_ALLOCATOR(PrefabEditManager, AZ::SystemAllocator, 0);

            PrefabEditManager();
            ~PrefabEditManager();

        private:
            // PrefabEditInterface...
            void EditOwningPrefab(AZ::EntityId entityId) override;
            bool IsOwningPrefabBeingEdited(AZ::EntityId entityId) override;
            bool IsOwningPrefabInEditStack(AZ::EntityId entityId) override;

            // EditorInteractionInterface...
            AZ::EntityId RedirectEntitySelection(AZ::EntityId entityId) override;

            class PrefabInstanceStack
            {
            public:
                void push(AZ::EntityId entityId);
                AZ::EntityId top();
                void pop();
                void push_bottom(AZ::EntityId entityId);
                bool empty();
                bool contains(AZ::EntityId entityId);

            private:
                AZStd::deque<AZ::EntityId> m_stack;
            };

            PrefabInstanceStack m_instanceEditStack;

            PrefabPublicInterface* m_prefabPublicInterface;
        };
    }
}
