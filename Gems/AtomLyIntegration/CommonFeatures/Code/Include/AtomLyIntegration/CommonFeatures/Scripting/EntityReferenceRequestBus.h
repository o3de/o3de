/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>

namespace AZ
{
    namespace Render
    {
        class EntityReferenceRequests
            : public ComponentBus
        {
        public:
            static const EBusHandlerPolicy HandlerPolicy = EBusHandlerPolicy::Single;

            virtual ~EntityReferenceRequests() = default;

            virtual AZStd::vector<EntityId> GetEntityReferences() const = 0;
        };

        using EntityReferenceRequestBus = AZ::EBus<EntityReferenceRequests>;
    }
}
