/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
            AZ_CLASS_ALLOCATOR(InstanceEntityMapper, AZ::SystemAllocator);
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
