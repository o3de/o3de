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

#include <AzToolsFramework/Prefab/Instance/Instance.h>

#include <AzCore/Interface/Interface.h>
#include <AzToolsFramework/Prefab/Instance/InstanceEntityMapper.h>

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
            return m_entityToInstanceMap.emplace(AZStd::make_pair(entityId, &instance)).second;
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
