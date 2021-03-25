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

#include <Source/NetworkTime/INetworkTime.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Console/IConsole.h>

namespace Multiplayer
{
    //! Implementation of the INetworkTime interface.
    class NetworkTime
        : public INetworkTime
    {
    public:
        NetworkTime();
        virtual ~NetworkTime();

        //! INetworkTime overrides.
        //! @{
        AZ::TimeMs ConvertFrameIdToTimeMs(ApplicationFrameId frameId) const override;
        ApplicationFrameId ConvertTimeMsToFrameId(AZ::TimeMs timeMs) const override;
        bool IsApplicationFrameIdRewound() const override;
        ApplicationFrameId GetApplicationFrameId() const override;
        ApplicationFrameId GetUnalteredApplicationFrameId() const override;
        void IncrementApplicationFrameId() override;
        void SyncRewindableEntityState() override;
        AzNetworking::ConnectionId GetRewindingConnectionId() const override;
        ApplicationFrameId GetApplicationFrameIdForRewindingConnection(AzNetworking::ConnectionId rewindConnectionId) const override;
        void AlterApplicationFrameId(ApplicationFrameId frameId, AzNetworking::ConnectionId rewindConnectionId) override;
        //! @}

    private:

        ApplicationFrameId m_applicationFrameId = ApplicationFrameId{0};
        ApplicationFrameId m_unalteredFrameId = ApplicationFrameId{0};
        AzNetworking::ConnectionId m_rewindingConnectionId = AzNetworking::InvalidConnectionId;
    };
}
