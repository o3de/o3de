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

#include <AzCore/Time/ITime.h>
#include <AzNetworking/ConnectionLayer/IConnection.h>

namespace Multiplayer
{
    //! This is a strong typedef for representing the number of application frames since application start.
    AZ_TYPE_SAFE_INTEGRAL(ApplicationFrameId, uint32_t);
    static constexpr ApplicationFrameId InvalidApplicationFrameId = ApplicationFrameId{0xFFFFFFFF};

    //! @class INetworkTime
    //! @brief This is an AZ::Interface<> for managing multiplayer specific time related operations.
    class INetworkTime
    {
    public:
        AZ_RTTI(INetworkTime, "{7D468063-255B-4FEE-86E1-6D750EEDD42A}");

        INetworkTime() = default;
        virtual ~INetworkTime() = default;

        //! Converts from an ApplicationFrameId to a corresponding TimeMs.
        //! @param frameId the ApplicationFrameId to convert to a TimeMs
        //! @return the TimeMs that corresponds to the provided ApplicationFrameId
        virtual AZ::TimeMs ConvertFrameIdToTimeMs(ApplicationFrameId frameId) const = 0;

        //! Converts from a TimeMs to an ApplicationFrameId.
        //! @param timeMs the TimeMs to convert to an ApplicationFrameId
        //! @return the ApplicationFrameId that corresponds to the provided TimeMs
        virtual ApplicationFrameId ConvertTimeMsToFrameId(AZ::TimeMs timeMs) const = 0;

        //! Returns true if the application frameId has been temporarily altered.
        //! @return true if the application frameId has been altered, false otherwise
        virtual bool IsApplicationFrameIdRewound() const = 0;

        //! Retrieves the applications current frameId (may be rewound on the server during backward reconciliation).
        //! @return the applications current frameId
        virtual ApplicationFrameId GetApplicationFrameId() const = 0;

        //! Retrieves the unaltered applications current frameId.
        //! @return the applications current frameId, unaltered by any scoped time instance
        virtual ApplicationFrameId GetUnalteredApplicationFrameId() const = 0;

        //! Increments the applications current frameId.
        virtual void IncrementApplicationFrameId() = 0;

        //! Synchronizes rewindable entity state for the current application time.
        virtual void SyncRewindableEntityState() = 0;

        //! Get the controlling connection that may be currently altering global game time.
        //! Note this abstraction is required at a relatively high level to allow for 'don't rewind the shooter' semantics
        //! @return the ConnectionId of the connection requesting the rewind operation
        virtual AzNetworking::ConnectionId GetRewindingConnectionId() const = 0;

        //! Get the controlling connection that may be currently altering global game time.
        //! Note this abstraction is required at a relatively high level to allow for 'don't rewind the shooter' semantics
        //! @param rewindConnectionId if this parameter matches the current rewindConnectionId, it will return the unaltered applicationFrameId
        //! @return the ApplicationFrameId taking into account the provided rewinding connectionId
        virtual ApplicationFrameId GetApplicationFrameIdForRewindingConnection(AzNetworking::ConnectionId rewindConnectionId) const = 0;

        //! Alters the current ApplicationFrameId and binds that alteration to the provided ConnectionId.
        //! @param frameId the new ApplicationFrameId to use
        //! @param rewindConnectionId the rewinding ConnectionId 
        virtual void AlterApplicationFrameId(ApplicationFrameId frameId, AzNetworking::ConnectionId rewindConnectionId) = 0;

        AZ_DISABLE_COPY_MOVE(INetworkTime);
    };

    // EBus wrapper for ScriptCanvas
    class INetworkTimeRequests
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
    };
    using INetworkTimeRequestBus = AZ::EBus<INetworkTime, INetworkTimeRequests>;

    //! @class ScopedAlterTime
    //! @brief This is a wrapper that temporarily adjusts global program time for backward reconciliation purposes.
    class ScopedAlterTime final
    {
    public:
        inline ScopedAlterTime(ApplicationFrameId frameId, AzNetworking::ConnectionId connectionId)
        {
            INetworkTime* time = AZ::Interface<INetworkTime>::Get();
            m_previousApplicationFrameId = time->GetApplicationFrameId();
            m_previousRewindConnectionId = time->GetRewindingConnectionId();
            time->AlterApplicationFrameId(frameId, connectionId);
        }
        inline ~ScopedAlterTime()
        {
            INetworkTime* time = AZ::Interface<INetworkTime>::Get();
            time->AlterApplicationFrameId(m_previousApplicationFrameId, m_previousRewindConnectionId);
        }
    private:
        ApplicationFrameId m_previousApplicationFrameId = InvalidApplicationFrameId;
        AzNetworking::ConnectionId m_previousRewindConnectionId = AzNetworking::InvalidConnectionId;
    };
}

AZ_TYPE_SAFE_INTEGRAL_SERIALIZEBINDING(Multiplayer::ApplicationFrameId);
