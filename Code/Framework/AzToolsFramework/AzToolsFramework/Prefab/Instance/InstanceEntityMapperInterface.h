/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/RTTI/RTTI.h>
#include <AzToolsFramework/Prefab/Instance/Instance.h>

namespace AZ
{
    class EntityId;
}

namespace AzToolsFramework
{
    namespace Prefab
    {
        class Instance;

        class InstanceEntityMapperInterface
        {
        public:
            AZ_RTTI(InstanceEntityMapperInterface, "{B0AF6374-4809-4304-91D6-3C94F670E2A4}");

            virtual InstanceOptionalReference FindOwningInstance(const AZ::EntityId& entityId) const = 0;

        protected:
            // Only the Instance class is allowed to register and unregister entities
            friend class Instance;
            friend class JsonInstanceSerializer;

            virtual bool RegisterEntityToInstance(const AZ::EntityId& entityId, Prefab::Instance& instance) = 0;
            virtual bool UnregisterEntity(const AZ::EntityId& entityId) = 0;
        };
    }
}
