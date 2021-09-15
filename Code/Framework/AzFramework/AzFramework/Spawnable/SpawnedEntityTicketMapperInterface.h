/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/EntityId.h>
#include <AzCore/RTTI/RTTI.h>

namespace AzFramework
{
    class SpawnedEntityTicketMapperInterface
    {
    public:
        AZ_RTTI(SpawnedEntityTicketMapperInterface, "{D407E96B-635C-44F2-B089-084EBAB8B036}");

        virtual void RemoveSpawnedEntity(AZ::EntityId entityId) = 0;
        virtual void AddSpawnedEntity(AZ::EntityId entityId, void* ticket) = 0;
    };
}
