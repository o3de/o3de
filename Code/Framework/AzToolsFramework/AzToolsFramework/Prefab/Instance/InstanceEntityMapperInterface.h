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

#include <AzCore/RTTI/RTTI.h>

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

            virtual bool RegisterEntityToInstance(const AZ::EntityId& entityId, Prefab::Instance& instance) = 0;
            virtual bool UnregisterEntity(const AZ::EntityId& entityId) = 0;
        };
    }
}
