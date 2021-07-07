/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef GM_CARRIER_DRILLER_H
#define GM_CARRIER_DRILLER_H

#include <GridMate/Types.h>
#include <GridMate/Carrier/Carrier.h>
#include <AzCore/Driller/Driller.h>

namespace GridMate
{
    namespace Debug
    {
        /**
         * Carrier driller
         * \note Be careful which buses you attach. The drillers work in Multi threaded environment and expect that
         * a driller mutex (DrillerManager::DrillerManager) will be automatically locked on every write.
         * Otherwise in output stream corruption will happen (even is the stream is thread safe).
         */
        class CarrierDriller
            : public AZ::Debug::Driller
            , public CarrierDrillerBus::Handler
        {
            int m_drillerTag;
        public:
            AZ_CLASS_ALLOCATOR(CarrierDriller, AZ::OSAllocator, 0);
            CarrierDriller();

            //////////////////////////////////////////////////////////////////////////
            // Driller
            const char*  GroupName() const override { return "GridMate"; }
            const char*  GetName() const override { return "CarrierDriller"; }
            const char*  GetDescription() const override { return "Drills Carrier/transport layer,traffic control, driver,etc."; }
            void         Start(const Param* params = nullptr, int numParams = 0) override;
            void         Stop() override;
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            // Carrier Driller Bus
            void OnUpdateStatistics(const GridMate::string& address, const TrafficControl::Statistics& lastSecond, const TrafficControl::Statistics& lifeTime, const TrafficControl::Statistics& effectiveLastSecond, const TrafficControl::Statistics& effectiveLifeTime) override;
            void OnConnectionStateChanged(Carrier* carrier, ConnectionID id, Carrier::ConnectionStates newState) override;
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            // Carrier Event Bus
            void OnIncomingConnection(Carrier* carrier, ConnectionID id) override;
            void OnFailedToConnect(Carrier* carrier, ConnectionID id, CarrierDisconnectReason reason) override;
            void OnConnectionEstablished(Carrier* carrier, ConnectionID id) override;
            void OnDisconnect(Carrier* carrier, ConnectionID id, CarrierDisconnectReason reason) override;
            void OnDriverError(Carrier* carrier, ConnectionID id, const DriverError& error) override;
            void OnSecurityError(Carrier* carrier, ConnectionID id, const SecurityError& error) override;
            //////////////////////////////////////////////////////////////////////////
        };
    }
}

#endif // GM_CARRIER_DRILLER_H
