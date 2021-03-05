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

#include "Multiplayer_precompiled.h"
#include <Source/Canvas/MultiplayerGameLiftLobbyCanvas.h>
#include <Source/Canvas/MultiplayerCanvasHelper.h>

namespace Multiplayer
{
    static const char* MultiplayerGameLiftLobbyCanvasName = "ui/Canvases/gamelift_lobby.uicanvas";
    static const char* MultiplayerGameLiftLobbyCreateServerContainerName = "CreateServerContainer";
    static const char* MultiplayerGameLiftLobbyJoinServerContainerName = "JoinServerContainer";
    static const char* MultiplayerGameLiftLobbyFlexMatchContainerName = "FlexMatchContainer";

    static const char* GameliftCreateContainerFleetIDInput = "FleetId";
    static const char* GameliftCreateContainerQueueNameInput = "QueueName";
    static const char* GameliftCreateContainerAliasIDInput = "AliasId";

    static const char* GameliftJoinContainerFleetIDInput = "JoinContainerFleetId";
    static const char* GameliftJoinContainerQueueNameInput = "JoinContainerQueueName";
    static const char* GameliftJoinContainerAliasIDInput = "JoinContainerAliasId";

    MultiplayerGameLiftLobbyCanvas::MultiplayerGameLiftLobbyCanvas(const MultiplayerGameLiftLobbyCanvasContext& context)
        : m_context(context)
    {
        m_canvasEntityId = LoadCanvas(MultiplayerGameLiftLobbyCanvasName);

        AZ_Error("MultiplayerLobbyComponent", m_canvasEntityId.IsValid(), "Missing UI file for GameLift Lobby.");

        m_context.JoinServerViewContext.ServerListingVector.emplace_back(69, 72, 70);
        m_context.JoinServerViewContext.ServerListingVector.emplace_back(73, 76, 74);
        m_context.JoinServerViewContext.ServerListingVector.emplace_back(77, 80, 78);
        m_context.JoinServerViewContext.ServerListingVector.emplace_back(81, 84, 82);
        m_context.JoinServerViewContext.ServerListingVector.emplace_back(85, 88, 86);

        m_joinServerScreen = aznew MultiplayerJoinServerView(m_context.JoinServerViewContext, m_canvasEntityId);
        m_flexMatchScreen = aznew MultiplayerGameLiftFlextMatchView(m_context.GameLiftFlexMatchViewContext, m_canvasEntityId);
        m_createServerScreen = aznew MultiplayerCreateServerView(m_context.CreateServerViewContext, m_canvasEntityId);

        UiCanvasNotificationBus::Handler::BusConnect(m_canvasEntityId);

        EBUS_EVENT_ID(m_canvasEntityId, UiCanvasBus, SetEnabled, false);
        SetElementEnabled(m_canvasEntityId, MultiplayerGameLiftLobbyCreateServerContainerName, true);
        SetElementEnabled(m_canvasEntityId, MultiplayerGameLiftLobbyJoinServerContainerName, false);
        SetElementEnabled(m_canvasEntityId, MultiplayerGameLiftLobbyFlexMatchContainerName, false);

        RefreshGameLiftConfig();
    }

    MultiplayerGameLiftLobbyCanvas::~MultiplayerGameLiftLobbyCanvas()
    {
        delete m_joinServerScreen;
        m_joinServerScreen = nullptr;

        delete m_createServerScreen;
        m_createServerScreen = nullptr;

        delete m_flexMatchScreen;
        m_flexMatchScreen = nullptr;

        UiCanvasNotificationBus::Handler::BusDisconnect(m_canvasEntityId);
        ReleaseCanvas(m_canvasEntityId);
        m_canvasEntityId.SetInvalid();
    }

    void MultiplayerGameLiftLobbyCanvas::Show()
    {
        EBUS_EVENT_ID(m_canvasEntityId, UiCanvasBus, SetEnabled, true);
    }

    void MultiplayerGameLiftLobbyCanvas::Hide()
    {
        EBUS_EVENT_ID(m_canvasEntityId, UiCanvasBus, SetEnabled, false);
    }

    void MultiplayerGameLiftLobbyCanvas::OnAction(AZ::EntityId entityId, const LyShine::ActionName& actionName)
    {
        AZ_UNUSED(entityId)
        if (actionName == "CreateServerRadioButtonOn")
        {
            SetElementEnabled(m_canvasEntityId, MultiplayerGameLiftLobbyCreateServerContainerName, true);
        }
        else if (actionName == "CreateServerRadioButtonOff")
        {
            SetElementEnabled(m_canvasEntityId, MultiplayerGameLiftLobbyCreateServerContainerName, false);
        }
        else if (actionName == "JoinServerRadioButtonOn")
        {
            SetElementEnabled(m_canvasEntityId, MultiplayerGameLiftLobbyJoinServerContainerName, true);
        }
        else if (actionName == "JoinServerRadioButtonOff")
        {
            SetElementEnabled(m_canvasEntityId, MultiplayerGameLiftLobbyJoinServerContainerName, false);
        }
        else if (actionName == "FlextMatchRadioButtonOn")
        {
            SetElementEnabled(m_canvasEntityId, MultiplayerGameLiftLobbyFlexMatchContainerName, true);
        }
        else if (actionName == "FlextMatchRadioButtonOff")
        {
            SetElementEnabled(m_canvasEntityId, MultiplayerGameLiftLobbyFlexMatchContainerName, false);
        }
        else if (actionName == "OnReturn")
        {
            m_context.OnReturnButtonClicked();
        }
        else if (actionName == "OnGameLiftConfigEdit")
        {
            SaveGameLiftConfig();
        }
        RefreshGameLiftConfig();
    }

    void MultiplayerGameLiftLobbyCanvas::RefreshGameLiftConfig()
    {
        if (IsElementEnabled(m_canvasEntityId, MultiplayerGameLiftLobbyCreateServerContainerName))
        {
            SetElementText(m_canvasEntityId, GameliftCreateContainerFleetIDInput, GetConsoleVarValue("gamelift_fleet_id"));
            SetElementText(m_canvasEntityId, GameliftCreateContainerQueueNameInput, GetConsoleVarValue("gamelift_queue_name"));
            SetElementText(m_canvasEntityId, GameliftCreateContainerAliasIDInput, GetConsoleVarValue("gamelift_alias_id"));
        }
        else if (IsElementEnabled(m_canvasEntityId, MultiplayerGameLiftLobbyJoinServerContainerName))
        {
            SetElementText(m_canvasEntityId, GameliftJoinContainerFleetIDInput, GetConsoleVarValue("gamelift_fleet_id"));
            SetElementText(m_canvasEntityId, GameliftJoinContainerQueueNameInput, GetConsoleVarValue("gamelift_queue_name"));
            SetElementText(m_canvasEntityId, GameliftJoinContainerAliasIDInput, GetConsoleVarValue("gamelift_alias_id"));
        }
    }

    void MultiplayerGameLiftLobbyCanvas::SaveGameLiftConfig()
    {
        LyShine::StringType param;
        if (IsElementEnabled(m_canvasEntityId, MultiplayerGameLiftLobbyCreateServerContainerName))
        { 
            param = GetElementText(m_canvasEntityId, GameliftCreateContainerFleetIDInput);
            SetConsoleVarValue("gamelift_fleet_id", param.c_str());

            param = GetElementText(m_canvasEntityId, GameliftCreateContainerQueueNameInput);
            SetConsoleVarValue("gamelift_queue_name", param.c_str());

            param = GetElementText(m_canvasEntityId, GameliftCreateContainerAliasIDInput);
            SetConsoleVarValue("gamelift_alias_id", param.c_str());
        }
        else if (IsElementEnabled(m_canvasEntityId, MultiplayerGameLiftLobbyJoinServerContainerName))
        {
            param = GetElementText(m_canvasEntityId, GameliftJoinContainerFleetIDInput);
            SetConsoleVarValue("gamelift_fleet_id", param.c_str());

            param = GetElementText(m_canvasEntityId, GameliftJoinContainerQueueNameInput);
            SetConsoleVarValue("gamelift_queue_name", param.c_str());

            param = GetElementText(m_canvasEntityId, GameliftJoinContainerAliasIDInput);
            SetConsoleVarValue("gamelift_alias_id", param.c_str());
        }
    }

    void MultiplayerGameLiftLobbyCanvas::DisplaySearchResults(const GridMate::GridSearch* search)
    {
        m_joinServerScreen->DisplaySearchResults(search);
    }

    LyShine::StringType MultiplayerGameLiftLobbyCanvas::GetMapName() const
    {
        return m_createServerScreen->GetMapName();
    }

    LyShine::StringType MultiplayerGameLiftLobbyCanvas::GetServerName() const
    {
        return m_createServerScreen->GetServerName();
    }

    void MultiplayerGameLiftLobbyCanvas::ClearSearchResults()
    {
        m_joinServerScreen->ClearSearchResults();
    }

    int MultiplayerGameLiftLobbyCanvas::GetSelectedServerResult()
    {
        return m_joinServerScreen->m_selectedServerResult;
    }
}
