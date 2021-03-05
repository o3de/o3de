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
#ifndef INCLUDE_NETWORKGRIDMATESYSTEMEVENTS_HEADER
#define INCLUDE_NETWORKGRIDMATESYSTEMEVENTS_HEADER

#pragma once

#include "Compatibility/GridMateRMI.h"

class IActorRMIRep;

namespace GridMate
{
    class NetworkSystemCallbacks
        : public AZ::EBusTraits
    {
    public:
        virtual ~NetworkSystemCallbacks() {}

        virtual void ActorRMISent(EntityId entityId, const IActorRMIRep& rep, uint32 paramsSize) { (void)entityId; (void)rep; (void)paramsSize; }
        virtual void ActorRMIReceived(EntityId entityId, const IActorRMIRep& rep, uint32 paramsSize) { (void)entityId; (void)rep; (void)paramsSize; }

        virtual void LegacyRMISent(EntityId entityId, const IRMIRep& rep, uint32 paramsSize) { (void)entityId; (void)rep; (void)paramsSize; }
        virtual void LegacyRMIReceived(EntityId entityId, const IRMIRep& rep, uint32 paramsSize) { (void)entityId; (void)rep; (void)paramsSize; }

        virtual void ScriptRMISent(uint32 paramsSize) { (void)paramsSize; }
        virtual void ScriptRMIReceived(uint32 paramsSize) { (void)paramsSize; }

        virtual void AspectSent(EntityId entityId, ASPECT_TYPE aspectBit, uint32 payloadSize) { (void)entityId; (void)aspectBit; (void)payloadSize; }
        virtual void AspectReceived(EntityId entityId, ASPECT_TYPE aspectBit, uint32 payloadSize) { (void)entityId; (void)aspectBit; (void)payloadSize; }
    };

    typedef AZ::EBus<NetworkSystemCallbacks> NetworkSystemEventBus;

    /*!
     * Acts as a sink for the session EBus.
     */
    class NetworkSystemEvents
        : public GridMate::NetworkSystemEventBus::Handler
    {
    public:

        NetworkSystemEvents();

        void Connect();
        void Disconnect();

        bool IsConnected() const { return m_connected; }

    public:

        void ActorRMISent(EntityId entityId, const IActorRMIRep& rep, uint32 paramsSize) override;
        void ActorRMIReceived(EntityId entityId, const IActorRMIRep& rep, uint32 paramsSize) override;

        void LegacyRMISent(EntityId entityId, const IRMIRep& rep, uint32 paramsSize) override;
        void LegacyRMIReceived(EntityId entityId, const IRMIRep& rep, uint32 paramsSize) override;

        void ScriptRMISent(uint32 paramsSize) override;
        void ScriptRMIReceived(uint32 paramsSize) override;

        void AspectSent(EntityId entityId, ASPECT_TYPE aspectBit, uint32 payloadSize) override;
        void AspectReceived(EntityId entityId, ASPECT_TYPE aspectBit, uint32 payloadSize) override;

    private:

        bool m_connected;
    };
} // namespace GridMate

#endif // INCLUDE_NETWORKGRIDMATESYSTEMEVENTS_HEADER
