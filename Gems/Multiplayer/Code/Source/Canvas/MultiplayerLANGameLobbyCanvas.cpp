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
#include <Source/Canvas/MultiplayerJoinServerView.h>
#include <Source/Canvas/MultiplayerCreateServerView.h>
#include <Source/Canvas/MultiplayerLANGameLobbyCanvas.h>
#include <Source/Canvas/MultiplayerCanvasHelper.h>

namespace Multiplayer
{
    static const char* MultiplayerLANGameLobbyCanvasName = "ui/Canvases/listing_lobby.uicanvas";

    MultiplayerLANGameLobbyCanvas::MultiplayerLANGameLobbyCanvas(const MultiplayerLANGameLobbyCanvasContext& context)
        : m_context(context)
    {
        m_canvasEntityId = LoadCanvas(MultiplayerLANGameLobbyCanvasName);
        AZ_Error("MultiplayerLobbyComponent", m_canvasEntityId.IsValid(), "Missing UI file for Server Listing Lobby.");

        m_context.JoinServerViewContext.ServerListingVector.emplace_back(10, 11, 32);
        m_context.JoinServerViewContext.ServerListingVector.emplace_back(12, 13, 33);
        m_context.JoinServerViewContext.ServerListingVector.emplace_back(14, 15, 34);
        m_context.JoinServerViewContext.ServerListingVector.emplace_back(16, 17, 35);
        m_context.JoinServerViewContext.ServerListingVector.emplace_back(18, 19, 36);

        m_joinServerScreen = aznew MultiplayerJoinServerView(m_context.JoinServerViewContext, m_canvasEntityId);
        m_createServerScreen = aznew MultiplayerCreateServerView(m_context.CreateServerViewContext, m_canvasEntityId);

        UiCanvasNotificationBus::Handler::BusConnect(m_canvasEntityId);
        EBUS_EVENT_ID(m_canvasEntityId, UiCanvasBus, SetEnabled, false);
        
    }

    MultiplayerLANGameLobbyCanvas::~MultiplayerLANGameLobbyCanvas()
    {
        delete m_joinServerScreen;
        m_joinServerScreen = nullptr;

        delete m_createServerScreen;
        m_createServerScreen = nullptr;

        UiCanvasNotificationBus::Handler::BusDisconnect(m_canvasEntityId);
        ReleaseCanvas(m_canvasEntityId);
        m_canvasEntityId.SetInvalid();
    }

    void MultiplayerLANGameLobbyCanvas::OnAction(AZ::EntityId entityId, const LyShine::ActionName& actionName)
    {
        AZ_UNUSED(entityId)
        if (actionName == "OnReturn")
        {
            m_context.OnReturnButtonClicked();
        }
    }

    void MultiplayerLANGameLobbyCanvas::Show()
    {
        EBUS_EVENT_ID(m_canvasEntityId, UiCanvasBus, SetEnabled, true);
    }

    void MultiplayerLANGameLobbyCanvas::Hide()
    {
        EBUS_EVENT_ID(m_canvasEntityId, UiCanvasBus, SetEnabled, false);
    }

    void MultiplayerLANGameLobbyCanvas::DisplaySearchResults(const GridMate::GridSearch* search)
    {
        m_joinServerScreen->DisplaySearchResults(search);
    }

    void MultiplayerLANGameLobbyCanvas::ClearSearchResults()
    {
        m_joinServerScreen->ClearSearchResults();
    }

    int MultiplayerLANGameLobbyCanvas::GetSelectedServerResult()
    {
        return m_joinServerScreen->m_selectedServerResult;
    }

    LyShine::StringType MultiplayerLANGameLobbyCanvas::GetMapName() const
    {
        return m_createServerScreen->GetMapName();
    }

    LyShine::StringType MultiplayerLANGameLobbyCanvas::GetServerName() const
    {
        return m_createServerScreen->GetServerName();
    }
}
