/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once
#include "MultiplayerDebugByteReporter.h"

#include <AzCore/Component/EntityId.h>
#include <AzCore/Interface/Interface.h>
#include <Debug/MultiplayerDebugPerEntityInterface.h>
#include <GridMate/Drillers/CarrierDriller.h>
#include <Multiplayer/MultiplayerTypes.h>

namespace MultiplayerDiagnostics
{
    /**
     * \brief GridMate network live analysis tool via ImGui.
     */
    class MultiplayerDebugPerEntityReporter
        : public AZ::Interface<MultiplayerIPerEntityStats>::Registrar
    {
    public:
        MultiplayerDebugPerEntityReporter() = default;
        ~MultiplayerDebugPerEntityReporter() override = default;

        // main update loop
        void OnImGuiUpdate();

        //! MultilayerIPerEntityStats
        // @{
        void RecordEntitySerializeStart(AzNetworking::SerializerMode mode, AZ::EntityId entityId, const char* entityName) override;
        void RecordComponentSerializeEnd(AzNetworking::SerializerMode mode, Multiplayer::NetComponentId netComponentId) override;
        void RecordEntitySerializeStop(AzNetworking::SerializerMode mode, AZ::EntityId entityId, const char* entityName) override;
        void RecordPropertySent(Multiplayer::NetComponentId netComponentId, Multiplayer::PropertyIndex propertyId, uint32_t totalBytes) override;
        void RecordPropertyReceived(Multiplayer::NetComponentId netComponentId, Multiplayer::PropertyIndex propertyId, uint32_t totalBytes) override;
        void RecordRpcSent(Multiplayer::NetComponentId netComponentId, Multiplayer::RpcIndex rpcId, uint32_t totalBytes) override;
        // }@

    private:

        AZStd::map<AZStd::string, EntityReporter> m_sendingEntityReports{};
        EntityReporter m_currentSendingEntityReport;

        AZStd::map<AZStd::string, EntityReporter> m_receivingEntityReports{};
        EntityReporter m_currentReceivingEntityReport;

        float m_replicatedStateKbpsWarn = 10.f;
        float m_replicatedStateMaxSizeWarn = 30.f;
    };
}
