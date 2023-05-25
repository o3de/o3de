/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/RTTI/RTTI.h>
#include <Multiplayer/MultiplayerEditorServerBus.h>

namespace Multiplayer::Automation
{
    //! Multiplayer Editor event handler for python automation
    //! This class will pick up Multiplayer Editor notifications and make sure they are forwarded and available for python automation tests
    class MultiplayerEditorAutomationHandler
        : public MultiplayerEditorServerNotificationBus::Handler
        , public AZ::BehaviorEBusHandler
    {
    public:
        AZ_EBUS_BEHAVIOR_BINDER(
            MultiplayerEditorAutomationHandler, "{CBA9A03D-ED7C-472E-B79F-1CCAB22D048C}", AZ::SystemAllocator,
            OnServerLaunched,
            OnServerLaunchFail,
            OnEditorConnectionAttempt,
            OnEditorConnectionAttemptsFailed,
            OnEditorSendingLevelData,
            OnEditorSendingLevelDataFailed,
            OnEditorSendingLevelDataSuccess,
            OnConnectToSimulationSuccess,
            OnConnectToSimulationFail,
            OnPlayModeEnd,
            OnEditorServerProcessStoppedUnexpectedly);

        static void Reflect(AZ::ReflectContext* context);

    private:
        //! MultiplayerEditorServerNotificationBus::Handler
        //! @{
        void OnServerLaunched() override;
        void OnServerLaunchFail() override;
        void OnEditorConnectionAttempt(uint16_t connectionAttempts, uint16_t maxAttempts) override;
        void OnEditorConnectionAttemptsFailed(uint16_t failedAttempts) override;
        void OnEditorSendingLevelData(uint32_t bytesSent, uint32_t bytesTotal) override;
        void OnEditorSendingLevelDataFailed() override;
        void OnEditorSendingLevelDataSuccess() override;
        void OnConnectToSimulationSuccess() override;
        void OnConnectToSimulationFail(uint16_t serverPort) override;
        void OnPlayModeEnd() override;
        void OnEditorServerProcessStoppedUnexpectedly() override;
        //! @}
    };

} // namespace Multiplayer::Automation
