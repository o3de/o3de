/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <GridMate/Drillers/CarrierDriller.h>
#include <AzCore/Math/Crc.h>

using namespace AZ::Debug;

namespace GridMate
{
    namespace Debug
    {
        //=========================================================================
        // CarrierDriller
        // [4/14/2011]
        //=========================================================================
        CarrierDriller::CarrierDriller()
        {
            m_drillerTag = AZ_CRC("CarrierDriller", 0x72a37d06);
        }

        //=========================================================================
        // Start
        // [4/14/2011]
        //=========================================================================
        void CarrierDriller::Start(const Param* params, int numParams)
        {
            (void)params;
            (void)numParams;
            CarrierDrillerBus::Handler::BusConnect();

            /* get carriers and output all the data
            m_output->BeginTag(m_drillerTag);
                m_output->Write(AZ_CRC("CarrierId"),carrier);
                m_output->BeginTag(AZ_CRC("StartDrill"));
                for(unsigned int iConn = 0; iConn < carrier->GetNumConnections(); ++iConn )
                {
                    ConnectionID connId = carrier->GetConnectionId(iConn);
                    m_output->BeginTag(AZ_CRC("Connection"));
                        m_output->Write(AZ_CRC("Id"),connId);
                        m_output->Write(AZ_CRC("Address"),carrier->ConnectionToAddress(connId));
                        m_output->Write(AZ_CRC("State"),static_cast<int>(carrier->GetConnectionState(connId)));
                    m_output->EndTag(AZ_CRC("Connection"));
                }
                m_output->EndTag(AZ_CRC("StartDrill"));
            m_output->EndTag(m_drillerTag);*/
        }

        //=========================================================================
        // Stop
        // [4/14/2011]
        //=========================================================================
        void CarrierDriller::Stop()
        {
            CarrierDrillerBus::Handler::BusDisconnect();
        }

        //=========================================================================
        // OnUpdateStatistics
        // [4/14/2011]
        //=========================================================================
        void CarrierDriller::OnUpdateStatistics(const AZStd::string& address, const TrafficControl::Statistics& lastSecond, const TrafficControl::Statistics& lifeTime, const TrafficControl::Statistics& effectiveLastSecond, const TrafficControl::Statistics& effectiveLifeTime)
        {
            m_output->BeginTag(m_drillerTag);
            m_output->BeginTag(AZ_CRC("Statistics", 0xe2d38b22));
            m_output->Write(AZ_CRC("Address", 0x0d4e6f81), address);
            m_output->BeginTag(AZ_CRC("LastSecond", 0x5e6ccbee));
            m_output->Write(AZ_CRC("DataSend", 0xae94c282), lastSecond.m_dataSend);
            m_output->Write(AZ_CRC("DataReceived", 0xd92f8e4b), lastSecond.m_dataReceived);
            m_output->Write(AZ_CRC("DataResend", 0xe44a3086), lastSecond.m_dataResend);
            m_output->Write(AZ_CRC("DataAcked", 0xbb5e5496), lastSecond.m_dataAcked);
            m_output->Write(AZ_CRC("PacketSend", 0x5b52fa79), lastSecond.m_packetSend);
            m_output->Write(AZ_CRC("PacketReceived", 0xf247dd9e), lastSecond.m_packetReceived);
            m_output->Write(AZ_CRC("PacketLost", 0xbc64441e), lastSecond.m_packetLost);
            m_output->Write(AZ_CRC("PacketAcked", 0x91c4b93a), lastSecond.m_packetAcked);
            m_output->Write(AZ_CRC("PacketLoss", 0x2200d1bd), lastSecond.m_packetLoss);
            m_output->Write(AZ_CRC("rtt", 0xb40f6cfb), lastSecond.m_rtt);
            //m_output->Write(AZ_CRC("flow"),lastSecond.m_flow);
            m_output->EndTag(AZ_CRC("LastSecond", 0x5e6ccbee));
            m_output->BeginTag(AZ_CRC("LifeTime", 0x3de73088));
            m_output->Write(AZ_CRC("DataSend", 0xae94c282), lifeTime.m_dataSend);
            m_output->Write(AZ_CRC("DataReceived", 0xd92f8e4b), lifeTime.m_dataReceived);
            m_output->Write(AZ_CRC("DataResend", 0xe44a3086), lifeTime.m_dataResend);
            m_output->Write(AZ_CRC("DataAcked", 0xbb5e5496), lifeTime.m_dataAcked);
            m_output->Write(AZ_CRC("PacketSend", 0x5b52fa79), lifeTime.m_packetSend);
            m_output->Write(AZ_CRC("PacketReceived", 0xf247dd9e), lifeTime.m_packetReceived);
            m_output->Write(AZ_CRC("PacketLost", 0xbc64441e), lifeTime.m_packetLost);
            m_output->Write(AZ_CRC("PacketAcked", 0x91c4b93a), lifeTime.m_packetAcked);
            m_output->Write(AZ_CRC("PacketLoss", 0x2200d1bd), lifeTime.m_packetLoss);
            m_output->Write(AZ_CRC("rtt", 0xb40f6cfb), lifeTime.m_rtt);
            //m_output->Write(AZ_CRC("flow"),lifeTime.m_flow);
            m_output->EndTag(AZ_CRC("LifeTime", 0x3de73088));
            m_output->BeginTag(AZ_CRC("EffectiveLastSecond", 0x8f84642f));
            m_output->Write(AZ_CRC("DataSend", 0xae94c282), effectiveLastSecond.m_dataSend);
            m_output->Write(AZ_CRC("DataReceived", 0xd92f8e4b), effectiveLastSecond.m_dataReceived);
            m_output->Write(AZ_CRC("DataResend", 0xe44a3086), effectiveLastSecond.m_dataResend);
            m_output->Write(AZ_CRC("DataAcked", 0xbb5e5496), effectiveLastSecond.m_dataAcked);
            m_output->Write(AZ_CRC("PacketSend", 0x5b52fa79), effectiveLastSecond.m_packetSend);
            m_output->Write(AZ_CRC("PacketReceived", 0xf247dd9e), effectiveLastSecond.m_packetReceived);
            m_output->Write(AZ_CRC("PacketLost", 0xbc64441e), effectiveLastSecond.m_packetLost);
            m_output->Write(AZ_CRC("PacketAcked", 0x91c4b93a), effectiveLastSecond.m_packetAcked);
            m_output->Write(AZ_CRC("PacketLoss", 0x2200d1bd), effectiveLastSecond.m_packetLoss);
            m_output->Write(AZ_CRC("rtt", 0xb40f6cfb), effectiveLastSecond.m_rtt);
            //m_output->Write(AZ_CRC("flow"),effectiveLastSecond.m_flow);
            m_output->EndTag(AZ_CRC("EffectiveLastSecond", 0x8f84642f));
            m_output->BeginTag(AZ_CRC("EffectiveLifeTime", 0x4644a47a));
            m_output->Write(AZ_CRC("DataSend", 0xae94c282), effectiveLifeTime.m_dataSend);
            m_output->Write(AZ_CRC("DataReceived", 0xd92f8e4b), effectiveLifeTime.m_dataReceived);
            m_output->Write(AZ_CRC("DataResend", 0xe44a3086), effectiveLifeTime.m_dataResend);
            m_output->Write(AZ_CRC("DataAcked", 0xbb5e5496), effectiveLifeTime.m_dataAcked);
            m_output->Write(AZ_CRC("PacketSend", 0x5b52fa79), effectiveLifeTime.m_packetSend);
            m_output->Write(AZ_CRC("PacketReceived", 0xf247dd9e), effectiveLifeTime.m_packetReceived);
            m_output->Write(AZ_CRC("PacketLost", 0xbc64441e), effectiveLifeTime.m_packetLost);
            m_output->Write(AZ_CRC("PacketAcked", 0x91c4b93a), effectiveLifeTime.m_packetAcked);
            m_output->Write(AZ_CRC("PacketLoss", 0x2200d1bd), effectiveLifeTime.m_packetLoss);
            m_output->Write(AZ_CRC("rtt", 0xb40f6cfb), effectiveLifeTime.m_rtt);
            //m_output->Write(AZ_CRC("flow"),effectiveLifeTime.m_flow);
            m_output->EndTag(AZ_CRC("EffectiveLifeTime", 0x4644a47a));
            m_output->EndTag(AZ_CRC("Statistics", 0xe2d38b22));
            m_output->EndTag(m_drillerTag);
        }
        //=========================================================================
        // OnConnectionStateChanged
        // [4/14/2011]
        //=========================================================================
        void CarrierDriller::OnConnectionStateChanged(Carrier* carrier, ConnectionID id, Carrier::ConnectionStates newState)
        {
            m_output->BeginTag(m_drillerTag);
            m_output->Write(AZ_CRC("CarrierId", 0x93f4bfbe), carrier);
            m_output->BeginTag(AZ_CRC("ConnectionState", 0x38a6a5da));
            m_output->Write(AZ_CRC("Id", 0xbf396750), id);
            m_output->Write(AZ_CRC("State", 0xa393d2fb), static_cast<int>(newState));
            m_output->EndTag(AZ_CRC("ConnectionState", 0x38a6a5da));
            m_output->EndTag(m_drillerTag);
        }
        //=========================================================================
        // OnIncomingConnection
        // [4/14/2011]
        //=========================================================================
        void CarrierDriller::OnIncomingConnection(Carrier* carrier, ConnectionID id)
        {
            m_output->BeginTag(m_drillerTag);
            m_output->Write(AZ_CRC("CarrierId", 0x93f4bfbe), carrier);
            m_output->BeginTag(AZ_CRC("IncomingConnection", 0x8c9d071a));
            m_output->Write(AZ_CRC("Id", 0xbf396750), id);
            m_output->Write(AZ_CRC("Address", 0x0d4e6f81), carrier->ConnectionToAddress(id));
            m_output->EndTag(AZ_CRC("IncomingConnection", 0x8c9d071a));
            m_output->EndTag(m_drillerTag);
        }
        //=========================================================================
        // OnFailedToConnect
        // [4/14/2011]
        //=========================================================================
        void CarrierDriller::OnFailedToConnect(Carrier* carrier, ConnectionID id, CarrierDisconnectReason reason)
        {
            m_output->BeginTag(m_drillerTag);
            m_output->Write(AZ_CRC("CarrierId", 0x93f4bfbe), carrier);
            m_output->BeginTag(AZ_CRC("FailedToConnect", 0xb6539549));
            m_output->Write(AZ_CRC("Id", 0xbf396750), id);
            m_output->Write(AZ_CRC("Reason", 0x3bb8880c), ReasonToString(reason));
            m_output->EndTag(AZ_CRC("FailedToConnect", 0xb6539549));
            m_output->EndTag(m_drillerTag);
        }
        //=========================================================================
        // OnConnectionEstablished
        // [4/14/2011]
        //=========================================================================
        void CarrierDriller::OnConnectionEstablished(Carrier* carrier, ConnectionID id)
        {
            m_output->BeginTag(m_drillerTag);
            m_output->Write(AZ_CRC("CarrierId", 0x93f4bfbe), carrier);
            m_output->BeginTag(AZ_CRC("ConnectionEstablished", 0xcde31aa7));
            m_output->Write(AZ_CRC("Id", 0xbf396750), id);
            m_output->EndTag(AZ_CRC("ConnectionEstablished", 0xcde31aa7));
            m_output->EndTag(m_drillerTag);
        }
        //=========================================================================
        // OnDisconnect
        // [4/14/2011]
        //=========================================================================
        void CarrierDriller::OnDisconnect(Carrier* carrier, ConnectionID id, CarrierDisconnectReason reason)
        {
            m_output->BeginTag(m_drillerTag);
            m_output->Write(AZ_CRC("CarrierId", 0x93f4bfbe), carrier);
            m_output->BeginTag(AZ_CRC("Disconnect", 0x003a4b91));
            m_output->Write(AZ_CRC("Id", 0xbf396750), id);
            m_output->Write(AZ_CRC("Reason", 0x3bb8880c), ReasonToString(reason));
            m_output->EndTag(AZ_CRC("Disconnect", 0x003a4b91));
            m_output->EndTag(m_drillerTag);
        }
        //=========================================================================
        // OnDriverError
        // [12/14/2016]
        //=========================================================================
        void CarrierDriller::OnDriverError(Carrier* carrier, ConnectionID id, const DriverError& error)
        {
            m_output->BeginTag(m_drillerTag);
            m_output->Write(AZ_CRC("CarrierId", 0x93f4bfbe), carrier);
            m_output->BeginTag(AZ_CRC("DriverError", 0xe7522aff));
            m_output->Write(AZ_CRC("Id", 0xbf396750), id);
            m_output->Write(AZ_CRC("ErrorCode", 0x499e660e), static_cast<int>(error.m_errorCode));
            m_output->EndTag(AZ_CRC("DriverError", 0xe7522aff));
            m_output->EndTag(m_drillerTag);
        }
        //=========================================================================
        // OnSecurityError
        //=========================================================================
        void CarrierDriller::OnSecurityError(Carrier* carrier, ConnectionID id, const SecurityError& error)
        {
            m_output->BeginTag(m_drillerTag);
            m_output->Write(AZ_CRC("CarrierId", 0x93f4bfbe), carrier);
            m_output->BeginTag(AZ_CRC("SecurityError", 0xdfe940ab));
            m_output->Write(AZ_CRC("Id", 0xbf396750), id);
            m_output->Write(AZ_CRC("ErrorCode", 0x499e660e), static_cast<int>(error.m_errorCode));
            m_output->EndTag(AZ_CRC("SecurityError", 0xdfe940ab));
            m_output->EndTag(m_drillerTag);
        }
    } // namespace Debug
} // namespace GridMate
