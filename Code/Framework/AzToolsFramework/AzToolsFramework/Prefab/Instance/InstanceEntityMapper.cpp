/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Interface/Interface.h>

#include <AzToolsFramework/Entity/PrefabEditorEntityOwnershipInterface.h>
#include <AzToolsFramework/Prefab/PrefabSystemComponentInterface.h>
#include <AzToolsFramework/Prefab/Instance/Instance.h>
#include <AzToolsFramework/Prefab/Instance/InstanceEntityMapper.h>
#include <AzToolsFramework/Prefab/Instance/InstanceEntityIdMapper.h>

namespace AzToolsFramework
{
    namespace Prefab
    {
        InstanceEntityMapper::InstanceEntityMapper()
        {
            AZ::Interface<InstanceEntityMapperInterface>::Register(this);
        }

        InstanceEntityMapper::~InstanceEntityMapper()
        {
            AZ::Interface<InstanceEntityMapperInterface>::Unregister(this);
        }

        bool InstanceEntityMapper::RegisterEntityToInstance(const AZ::EntityId& entityId, Instance& instance)
        {
            auto findResult = m_entityToInstanceMap.find(entityId);

            if (findResult != m_entityToInstanceMap.end())
            {
                AZ_Warning("Prefab", false, "Entity with id '%llu' is already registered to an instance whose source path is '%s'.",
                    static_cast<AZ::u64>(entityId), findResult->second->GetTemplateSourcePath().c_str());
                return false;
            }
            else
            {
                return m_entityToInstanceMap.emplace(AZStd::make_pair(entityId, &instance)).second;
            }
        }

        bool InstanceEntityMapper::UnregisterEntity(const AZ::EntityId& entityId)
        {
            return m_entityToInstanceMap.erase(entityId) != 0;
        }

        InstanceOptionalReference InstanceEntityMapper::FindOwningInstance(const AZ::EntityId& entityId) const
        {
            auto findResult = m_entityToInstanceMap.find(entityId);

            if (findResult != m_entityToInstanceMap.end())
            {
                Instance* foundInstance = findResult->second;
                if (foundInstance != nullptr)
                {
                    return *foundInstance;
                }
            }

            return AZStd::nullopt;
        }
    }
}
