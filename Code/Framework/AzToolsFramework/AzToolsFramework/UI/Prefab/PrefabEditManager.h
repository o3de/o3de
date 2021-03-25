/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
