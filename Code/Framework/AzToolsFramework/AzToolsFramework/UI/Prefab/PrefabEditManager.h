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

namespace AzToolsFramework
{
    namespace Prefab
    {
        class PrefabEditManager final
            : private PrefabEditInterface
        {
        public:
            AZ_CLASS_ALLOCATOR(PrefabEditManager, AZ::SystemAllocator, 0);

            PrefabEditManager();
            ~PrefabEditManager();

        private:
            // PrefabEditInterface...
            void EditOwningPrefab(AZ::EntityId entityId) override;
            bool IsOwningPrefabBeingEdited(AZ::EntityId entityId) override;

            AZ::EntityId m_instanceBeingEdited;

            PrefabPublicInterface* m_prefabPublicInterface;
        };
    }
}
