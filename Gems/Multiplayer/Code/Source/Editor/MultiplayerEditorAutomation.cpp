/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "MultiplayerEditorAutomation.h"

namespace Multiplayer::Automation
{
    void MultiplayerEditorAutomationHandler::Reflect(AZ::ReflectContext* context)
    {
        if (auto* behaviorContext = azdynamic_cast<AZ::BehaviorContext*>(context))
        {
            // Exposes bus handler to Python automation in the form of azlmbr.<module name>.<ebus name>Handler()
            // Example Python code:
            //     handler = azlmbr.multiplayer.MultiplayerEditorServerNotificationBusHandler()
            //     handler.connect()
            //     handler.add_callback("OnServerLaunched", _on_server_launched)
            behaviorContext->EBus<MultiplayerEditorServerNotificationBus>("MultiplayerEditorServerNotificationBus")
                ->Attribute(AZ::Script::Attributes::Module, "multiplayer")
                ->Attribute(AZ::Script::Attributes::Category, "Multiplayer")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                ->Handler<MultiplayerEditorAutomationHandler>();
        }
    }

    void MultiplayerEditorAutomationHandler::OnServerLaunched()
    {
        Call(FN_OnServerLaunched);
    }

    void MultiplayerEditorAutomationHandler::OnServerLaunchFail()
    {
        Call(FN_OnServerLaunchFail);
    }

    void MultiplayerEditorAutomationHandler::OnEditorConnectionAttempt(uint16_t connectionAttempts, uint16_t maxAttempts)
    {
        Call(FN_OnEditorConnectionAttempt, connectionAttempts, maxAttempts);
    }

    void MultiplayerEditorAutomationHandler::OnEditorConnectionAttemptsFailed(uint16_t failedAttempts)
    {
        Call(FN_OnEditorConnectionAttemptsFailed, failedAttempts);
    }

    void MultiplayerEditorAutomationHandler::OnEditorSendingLevelData(uint32_t bytesSent, uint32_t bytesTotal)
    {
        Call(FN_OnEditorSendingLevelData, bytesSent, bytesTotal);
    }

    void MultiplayerEditorAutomationHandler::OnEditorSendingLevelDataFailed()
    {
        Call(FN_OnEditorSendingLevelDataFailed);
    }

    void MultiplayerEditorAutomationHandler::OnEditorSendingLevelDataSuccess()
    {
        Call(FN_OnEditorSendingLevelDataSuccess);
    }

    void MultiplayerEditorAutomationHandler::OnConnectToSimulationSuccess()
    {
        Call(FN_OnConnectToSimulationSuccess);
    }

    void MultiplayerEditorAutomationHandler::OnConnectToSimulationFail(uint16_t serverPort)
    {
        Call(FN_OnConnectToSimulationFail, serverPort);
    }

    void MultiplayerEditorAutomationHandler::OnPlayModeEnd()
    {
        Call(FN_OnPlayModeEnd);
    }

    void MultiplayerEditorAutomationHandler::OnEditorServerProcessStoppedUnexpectedly()
    {
        Call(FN_OnEditorServerProcessStoppedUnexpectedly);
    }
} // namespace Multiplayer::Automation
