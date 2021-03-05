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
#include <Source/Canvas/MultiplayerCanvasHelper.h>
#include <Source/Canvas/MultiplayerBusyAndErrorCanvas.h>

namespace Multiplayer
{
    static const char* ErrorWindow = "ErrorWindow";
    static const char* ErrorMessage = "ErrorMessage";
    static const char* BusyScreen = "BusyScreen";
    static const char* MultiplayeBusyAndErrorCanvasName = "ui/Canvases/busy_error.uicanvas";


    MultiplayerBusyAndErrorCanvas::MultiplayerBusyAndErrorCanvas(const MultiplayerBusyAndErrorCanvasContext& context)
        : m_context(context)
        , m_isShowingBusy(false)
        , m_isShowingError(false)
    {
        m_canvasEntityId = LoadCanvas(MultiplayeBusyAndErrorCanvasName);
        AZ_Error("MultiplayerLobbyComponent", m_canvasEntityId.IsValid(), "Missing UI file for Busy and Error Canvas.");

        UiCanvasNotificationBus::Handler::BusConnect(m_canvasEntityId);

        SetElementEnabled(m_canvasEntityId, ErrorWindow, false);
        SetElementEnabled(m_canvasEntityId, BusyScreen, false);
    }

    MultiplayerBusyAndErrorCanvas::~MultiplayerBusyAndErrorCanvas()
    {
        UiCanvasNotificationBus::Handler::BusDisconnect(m_canvasEntityId);
    }

    void MultiplayerBusyAndErrorCanvas::OnAction(AZ::EntityId entityId, const LyShine::ActionName& actionName)
    {
        AZ_UNUSED(entityId);
        if (actionName == "OnDismissErrorMessage")
        {
            m_context.OnDismissErrroWindowButtonClicked(false);
        }
    }

    void MultiplayerBusyAndErrorCanvas::ShowError(const char* message)
    {
        if (m_isShowingBusy)
        {
            DismissBusyScreen();
        }
        if (!m_isShowingError)
        {
            m_isShowingError = true;

            SetElementEnabled(m_canvasEntityId, ErrorWindow, true);
            SetElementText(m_canvasEntityId, ErrorMessage, message);
        }
        else
        {
            m_errorMessageQueue.emplace_back(message);
        }
    }


    void MultiplayerBusyAndErrorCanvas::ShowQueuedErrorMessage()
    {
        if (!m_errorMessageQueue.empty())
        {
            AZStd::string errorMessage = m_errorMessageQueue.front();
            m_errorMessageQueue.erase(m_errorMessageQueue.begin());

            ShowError(errorMessage.c_str());
        }
    }

    void MultiplayerBusyAndErrorCanvas::DismissError(bool force)
    {
        if (m_isShowingError || force)
        {
            m_isShowingError = false;

            SetElementEnabled(m_canvasEntityId, ErrorWindow, false);

            if (force)
            {
                m_errorMessageQueue.clear();
            }
            else
            {
                ShowQueuedErrorMessage();
            }
        }
    }

    void MultiplayerBusyAndErrorCanvas::ShowBusyScreen()
    {
        if (!m_isShowingBusy)
        {
            m_isShowingBusy = true;
            SetElementEnabled(m_canvasEntityId, BusyScreen, true);
        }
    }

    void MultiplayerBusyAndErrorCanvas::DismissBusyScreen(bool force)
    {
        if (m_isShowingBusy || force)
        {
            m_isShowingBusy = false;

            SetElementEnabled(m_canvasEntityId, BusyScreen, false);
        }
    }
}
