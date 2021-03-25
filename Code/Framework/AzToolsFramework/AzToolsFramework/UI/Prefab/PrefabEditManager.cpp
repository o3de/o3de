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

#include <AzToolsFramework/UI/Prefab/PrefabEditManager.h>

#include <AzCore/Serialization/SerializeContext.h>

namespace AzToolsFramework
{
    namespace Prefab
    {
        PrefabEditManager::PrefabEditManager()
        {
            m_prefabPublicInterface = AZ::Interface<PrefabPublicInterface>::Get();

            if (m_prefabPublicInterface == nullptr)
            {
                AZ_Assert(false, "Prefab - could not get PrefabPublicInterface on PrefabEditManager construction.");
                return;
            }

            AZ::Interface<PrefabEditInterface>::Register(this);
        }

        PrefabEditManager::~PrefabEditManager()
        {
            AZ::Interface<PrefabEditInterface>::Unregister(this);
        }

        void PrefabEditManager::EditOwningPrefab(AZ::EntityId entityId)
        {
            m_instanceBeingEdited = m_prefabPublicInterface->GetInstanceContainerEntityId(entityId);
        }

        bool PrefabEditManager::IsOwningPrefabBeingEdited(AZ::EntityId entityId)
        {
            AZ::EntityId containerEntity = m_prefabPublicInterface->GetInstanceContainerEntityId(entityId);
            return m_instanceBeingEdited == containerEntity;
        }
    }
}
