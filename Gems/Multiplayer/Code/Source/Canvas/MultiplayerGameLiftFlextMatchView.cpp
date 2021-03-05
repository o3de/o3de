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
#include <Source/Canvas/MultiplayerGameLiftFlextMatchView.h>
#include <Source/Canvas/MultiplayerCanvasHelper.h>

namespace Multiplayer
{
    const char* MultiplayerGameLiftMatchmakingConfig = "MatchmakingConfigTextBox";

    MultiplayerGameLiftFlextMatchView::MultiplayerGameLiftFlextMatchView(const MultiplayerGameLiftFlextMatchViewContext& context, const AZ::EntityId canvasEntityId)
        : m_context(context)
        , m_canvasEntityId(canvasEntityId)
    {
        UiCanvasNotificationBus::Handler::BusConnect(m_canvasEntityId);

        LyShine::StringType configName = GetConsoleVarValue("gamelift_matchmaking_config_name");
        if (!configName.empty())
        {
            SetElementText(m_canvasEntityId, MultiplayerGameLiftMatchmakingConfig, configName.c_str());
        }
        else
        {
            SetElementText(m_canvasEntityId, MultiplayerGameLiftMatchmakingConfig, m_context.DefaultMatchmakingConfig.c_str());
            SaveMatchmakingConfigName();
        }
    }

    MultiplayerGameLiftFlextMatchView::~MultiplayerGameLiftFlextMatchView()
    {
        UiCanvasNotificationBus::Handler::BusDisconnect(m_canvasEntityId);
    }

    void MultiplayerGameLiftFlextMatchView::OnAction(AZ::EntityId entityId, const LyShine::ActionName& actionName)
    {
        AZ_UNUSED(entityId)
        if (actionName == "OnStartMatchmaking")
        {
            SaveMatchmakingConfigName();
            m_context.OnStartMatchmakingButtonClicked();
        }
    }

    void MultiplayerGameLiftFlextMatchView::SaveMatchmakingConfigName()
    {
        LyShine::StringType param;

        param = GetElementText(m_canvasEntityId, MultiplayerGameLiftMatchmakingConfig);;
        SetConsoleVarValue("gamelift_matchmaking_config_name", param.c_str());
        
    }
}
