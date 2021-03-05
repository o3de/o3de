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
#include <Source/Canvas/MultiplayerDedicatedHostTypeSelectionCanvas.h>
#include <Source/Canvas/MultiplayerCanvasHelper.h>

namespace Multiplayer
{
    static const char* MultiplayerDedicatedHostTypeSelectionCanvasName = "ui/Canvases/selection_lobby.uicanvas";
    
    static const char* GameLiftConfigWindow = "GameLiftConfig";
    static const char* GameliftAWSAccesKeyInput = "AWSAccessKey";
    static const char* GameliftAWSSecretKeyInput = "AWSSecretKey";
    static const char* GameliftAWSRegionInput = "AWSRegion";
    static const char* GameliftEndPointInput = "EndPoint";

    static const char* GameliftPlayerIDInput = "PlayerId";
    static const char* GameliftButton = "GameLiftButton";

    MultiplayerDedicatedHostTypeSelectionCanvas::MultiplayerDedicatedHostTypeSelectionCanvas(const MultiplayerDedicatedHostTypeSelectionCanvasContext& context)
        : m_context(context)
        , m_isShowingGameLiftConfig(false)
    {
        m_canvasEntityId = LoadCanvas(MultiplayerDedicatedHostTypeSelectionCanvasName);
        AZ_Error("MultiplayerLobbyComponent", m_canvasEntityId.IsValid(), "Missing UI file for ServerType Selection Lobby.");

        UiCanvasNotificationBus::Handler::BusConnect(m_canvasEntityId);

#if defined(BUILD_GAMELIFT_CLIENT)
        SetElementInputEnabled(m_canvasEntityId, GameliftButton, true);
#else
        SetElementInputEnabled(m_canvasEntityId, GameliftButton, false);
#endif

        SetElementEnabled(m_canvasEntityId, GameLiftConfigWindow, false);
    }

    MultiplayerDedicatedHostTypeSelectionCanvas::~MultiplayerDedicatedHostTypeSelectionCanvas()
    {
        UiCanvasNotificationBus::Handler::BusDisconnect();
        ReleaseCanvas(m_canvasEntityId);
    }

    void MultiplayerDedicatedHostTypeSelectionCanvas::Show()
    {
        EBUS_EVENT_ID(m_canvasEntityId, UiCanvasBus, SetEnabled, true);
    }

    void MultiplayerDedicatedHostTypeSelectionCanvas::Hide()
    {
        EBUS_EVENT_ID(m_canvasEntityId, UiCanvasBus, SetEnabled, false);
    }

    void MultiplayerDedicatedHostTypeSelectionCanvas::OnAction(AZ::EntityId entityId, const LyShine::ActionName& actionName)
    {
        AZ_UNUSED(entityId)
        if (actionName == "LANButtonClicked")
        {
            m_context.OnLANButtonClicked();
        }
        else if (actionName == "GameliftButtonClicked")
        {
            ShowGameLiftConfig();
        }
        else if (actionName == "OnGameliftConnect")
        {
            SaveGameLiftConfig();
            DismissGameLiftConfig();
            m_context.OnGameLiftConnectButtonClicked();
        }
        else if (actionName == "OnGameliftCancel")
        {
            DismissGameLiftConfig();
        }
    }

    void MultiplayerDedicatedHostTypeSelectionCanvas::ShowGameLiftConfig()
    {
        if (!m_isShowingGameLiftConfig)
        {
            m_isShowingGameLiftConfig = true;

            SetElementEnabled(m_canvasEntityId, GameLiftConfigWindow, true);
            SetElementText(m_canvasEntityId, GameliftAWSAccesKeyInput, GetConsoleVarValue("gamelift_aws_access_key"));
            SetElementText(m_canvasEntityId, GameliftAWSSecretKeyInput, GetConsoleVarValue("gamelift_aws_secret_key"));
            SetElementText(m_canvasEntityId, GameliftAWSRegionInput, GetConsoleVarValue("gamelift_aws_region"));
            SetElementText(m_canvasEntityId, GameliftPlayerIDInput, GetConsoleVarValue("gamelift_player_id"));
            SetElementText(m_canvasEntityId, GameliftEndPointInput, GetConsoleVarValue("gamelift_endpoint"));
        }
    }

    void MultiplayerDedicatedHostTypeSelectionCanvas::DismissGameLiftConfig()
    {
        if (m_isShowingGameLiftConfig)
        {
            m_isShowingGameLiftConfig = false;
            SetElementEnabled(m_canvasEntityId, GameLiftConfigWindow, false);
        }
    }

    void MultiplayerDedicatedHostTypeSelectionCanvas::SaveGameLiftConfig()
    {
        LyShine::StringType param;

        param = GetElementText(m_canvasEntityId, GameliftAWSAccesKeyInput);
        SetConsoleVarValue("gamelift_aws_access_key", param.c_str());

        param = GetElementText(m_canvasEntityId, GameliftAWSSecretKeyInput);
        SetConsoleVarValue("gamelift_aws_secret_key", param.c_str());

        param = GetElementText(m_canvasEntityId, GameliftAWSRegionInput);
        SetConsoleVarValue("gamelift_aws_region", param.c_str());

        param = GetElementText(m_canvasEntityId, GameliftEndPointInput);
        SetConsoleVarValue("gamelift_endpoint", param.c_str());

        param = GetElementText(m_canvasEntityId, GameliftPlayerIDInput);
        SetConsoleVarValue("gamelift_player_id", param.c_str());
    }
}
