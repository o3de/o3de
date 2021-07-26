/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once
#include <GridMate/Drillers/ReplicaDriller.h>
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
        : public AZ::Interface<MultilayerIPerEntityStats>::Registrar
    {
    public:
        MultiplayerDebugPerEntityReporter();
        ~MultiplayerDebugPerEntityReporter() override;

        // main update loop
        void OnImGuiUpdate();

        //! MultilayerIPerEntityStats
        // @{
        void RecordEntitySerializeStart(AZ::EntityId entityId, const char* entityName);
        void RecordEntitySerializeStop(AZ::EntityId entityId, const char* entityName);
        void RecordPropertySent(Multiplayer::NetComponentId netComponentId, Multiplayer::PropertyIndex propertyId, uint32_t totalBytes) override;
        void RecordPropertyReceived(Multiplayer::NetComponentId netComponentId, Multiplayer::PropertyIndex propertyId, uint32_t totalBytes) override;
        // }@

        /*void OnReceiveReplicaBegin(AZ::EntityId entityId, const void* data, size_t len) override;
        void OnReceiveReplicaEnd(AZ::EntityId entityId) override;
        void OnReceiveReplicaChunkEnd(GridMate::ReplicaChunkBase* chunk, AZ::u32 chunkIndex) override;
        void OnReceiveDataSet(GridMate::ReplicaChunkBase* chunk, AZ::u32 chunkIndex, GridMate::DataSetBase* dataSet, GridMate::PeerId from, GridMate::PeerId to, const void* data, size_t len) override;
        void OnReceiveRpc(GridMate::ReplicaChunkBase* chunk, AZ::u32 chunkIndex, GridMate::Internal::RpcRequest* rpc, GridMate::PeerId from, GridMate::PeerId to, const void* data, size_t len) override;*/

        // ReplicaDrillerBus - sending

        /*void OnSendReplicaBegin(AZ::EntityId entityId) override;
        void OnSendReplicaEnd(AZ::EntityId entityId, const void* data, size_t len) override;
        void OnSendReplicaChunkEnd(GridMate::ReplicaChunkBase* chunk, AZ::u32 chunkIndex, const void* data, size_t len) override;
        void OnSendDataSet(GridMate::ReplicaChunkBase* chunk, AZ::u32 chunkIndex, GridMate::DataSetBase* dataSet, GridMate::PeerId from, GridMate::PeerId to, const void* data, size_t len) override;
        void OnSendRpc(GridMate::ReplicaChunkBase* chunk, AZ::u32 chunkIndex, GridMate::Internal::RpcRequest* rpc, GridMate::PeerId from, GridMate::PeerId to, const void* data, size_t len) override;*/

        //// CarrierDrillerBus
        //void OnIncomingConnection (GridMate::Carrier* carrier, GridMate::ConnectionID id) override;
        //void OnFailedToConnect (GridMate::Carrier* carrier,
        //    GridMate::ConnectionID id,
        //    GridMate::CarrierDisconnectReason reason) override;
        //void OnConnectionEstablished (GridMate::Carrier* carrier, GridMate::ConnectionID id) override;
        //void OnDisconnect (GridMate::Carrier* carrier,
        //    GridMate::ConnectionID id,
        //    GridMate::CarrierDisconnectReason reason) override;
        //void OnDriverError (GridMate::Carrier* carrier,
        //    GridMate::ConnectionID id,
        //    const GridMate::DriverError& error) override;
        //void OnSecurityError (GridMate::Carrier* carrier,
        //    GridMate::ConnectionID id,
        //    const GridMate::SecurityError& error) override;
        //void OnUpdateStatistics (const GridMate::string& address,
        //    const GridMate::TrafficControl::Statistics& lastSecond,
        //    const GridMate::TrafficControl::Statistics& lifeTime,
        //    const GridMate::TrafficControl::Statistics& effectiveLastSecond,
        //    const GridMate::TrafficControl::Statistics& effectiveLifeTime) override;
        //void OnConnectionStateChanged (GridMate::Carrier* carrier,
        //    GridMate::ConnectionID id,
        //    GridMate::Carrier::ConnectionStates newState) override;

    private:

        bool m_showServerReportWindow = false;
        bool m_isTrackingMessages = false;

        AZStd::map<AZStd::string, EntityReporter> m_sendingEntityReports{};
        EntityReporter m_currentSendingEntityReport;

        AZStd::map<AZStd::string, EntityReporter> m_receivingEntityReports{};
        EntityReporter m_currentReceivingEntityReport;

        float m_replicatedStateKbpsWarn = 10.f;
        float m_replicatedStateMaxSizeWarn = 30.f;

        void UpdateTrafficStatistics();
        AZStd::map<GridMate::string, GridMate::TrafficControl::Statistics> m_lastSecondStats;
    };
}
