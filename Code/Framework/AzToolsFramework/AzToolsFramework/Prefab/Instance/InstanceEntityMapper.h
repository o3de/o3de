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
#include <AzCore/std/containers/unordered_map.h>
#include <AzToolsFramework/Prefab/Instance/InstanceEntityMapperInterface.h>
namespace AzToolsFramework
{
    namespace Prefab
    {
        class InstanceEntityMapper final
            : public InstanceEntityMapperInterface
        {
        public:
            AZ_CLASS_ALLOCATOR(InstanceEntityMapper, AZ::SystemAllocator, 0);
            AZ_RTTI(InstanceEntityMapper, "{6B11401C-A852-45E5-9016-A90BE285C932}", InstanceEntityMapperInterface);

            InstanceEntityMapper();
            ~InstanceEntityMapper();

            InstanceOptionalReference FindOwningInstance(const AZ::EntityId& entityId) const override;

        protected:
            bool RegisterEntityToInstance(const AZ::EntityId& entityId, Instance& instance) override;
            bool UnregisterEntity(const AZ::EntityId& entityId) override;

        private:
            AZStd::unordered_map<AZ::EntityId, Instance*> m_entityToInstanceMap;
        };
    }
}
