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
    class MultiplayerIPerEntityStats
    {
    public:
        AZ_RTTI(MultiplayerIPerEntityStats, "{91A1E4F0-8AE6-44B2-89DF-DA34134C408A}");

        virtual void RecordEntitySerializeStart(AzNetworking::SerializerMode mode, AZ::EntityId entityId, const char* entityName) = 0;
        virtual void RecordComponentSerializeEnd(AzNetworking::SerializerMode mode, Multiplayer::NetComponentId netComponentId) = 0;
        virtual void RecordEntitySerializeStop(AzNetworking::SerializerMode mode, AZ::EntityId entityId, const char* entityName) = 0;
        virtual void RecordPropertySent(Multiplayer::NetComponentId netComponentId, Multiplayer::PropertyIndex propertyId, uint32_t totalBytes) = 0;
        virtual void RecordPropertyReceived(Multiplayer::NetComponentId netComponentId, Multiplayer::PropertyIndex propertyId, uint32_t totalBytes) = 0;
        virtual void RecordRpcSent(Multiplayer::NetComponentId netComponentId, Multiplayer::RpcIndex rpcId, uint32_t totalBytes) = 0;
        virtual void RecordRpcReceived(Multiplayer::NetComponentId netComponentId, Multiplayer::RpcIndex rpcId, uint32_t totalBytes) = 0;
    };
}
