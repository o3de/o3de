/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzNetworking/Utilities/TimedThread.h>

namespace AZ
{
    class Name;
}

namespace RemoteTools
{
    class RemoteToolsSystemComponent;

    //! @class ToolingJoinThread
    //! @brief A class for polling a connection to the host target
    class RemoteToolsJoinThread final : public AzNetworking::TimedThread
    {
    public:
        RemoteToolsJoinThread(int updateRate, RemoteToolsSystemComponent* component);
        ~RemoteToolsJoinThread() override;

    private:
        AZ_DISABLE_COPY_MOVE(RemoteToolsJoinThread);

        //! Invoked on thread start
        void OnStart() override{}

        //! Invoked on thread stop
        void OnStop() override{}

        //! Invoked on thread update to poll for a target host to join
        //! @param updateRateMs The amount of time the thread can spend in OnUpdate in ms
        void OnUpdate(AZ::TimeMs updateRateMs) override;

        RemoteToolsSystemComponent* m_remoteToolsComponent;
    };
} // namespace RemoteTools
