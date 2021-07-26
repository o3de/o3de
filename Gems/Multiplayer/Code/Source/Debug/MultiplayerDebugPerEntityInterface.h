/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/EntityId.h>
#include <Multiplayer/MultiplayerTypes.h>

namespace MultiplayerDiagnostics
{
    class MultilayerIPerEntityStats
    {
    public:
        virtual ~MultilayerIPerEntityStats();

        virtual void RecordEntitySerializeStart(AZ::EntityId entityId, const char* entityName) = 0;
        virtual void RecordEntitySerializeStop(AZ::EntityId entityId, const char* entityName) = 0;
        virtual void RecordPropertySent(Multiplayer::NetComponentId netComponentId, Multiplayer::PropertyIndex propertyId, uint32_t totalBytes);
        virtual void RecordPropertyReceived(Multiplayer::NetComponentId netComponentId, Multiplayer::PropertyIndex propertyId, uint32_t totalBytes);
    };
}
