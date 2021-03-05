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
#include <Source/Canvas/MultiplayerCreateServerView.h>

namespace Multiplayer
{
    static const char* ServerNameTextBox = "ServerNameTextBox";
    static const char* MapNameTextBox = "MapNameTextBox";

    MultiplayerCreateServerView::MultiplayerCreateServerView(const MultiplayerCreateServerViewContext& context, const AZ::EntityId canvasEntityId)
        : m_context(context)
        , m_canvasEntityId(canvasEntityId)
    {
        UiCanvasNotificationBus::Handler::BusConnect(m_canvasEntityId);

        SetElementText(m_canvasEntityId, ServerNameTextBox, m_context.DefaultServerName.c_str());
        SetElementText(m_canvasEntityId, MapNameTextBox, m_context.DefaultMapName.c_str());

    }

    MultiplayerCreateServerView::~MultiplayerCreateServerView()
    {
        UiCanvasNotificationBus::Handler::BusDisconnect(m_canvasEntityId);
    }

    void MultiplayerCreateServerView::OnAction(AZ::EntityId entityId, const LyShine::ActionName& actionName)
    {
        AZ_UNUSED(entityId)
        if (actionName == "OnCreateServer")
        {
            m_context.OnCreateServerButtonClicked();
        }
    }

    LyShine::StringType MultiplayerCreateServerView::GetMapName() const
    {
        return GetElementText(m_canvasEntityId, MapNameTextBox);
    }

    LyShine::StringType MultiplayerCreateServerView::GetServerName() const
    {
        return GetElementText(m_canvasEntityId, ServerNameTextBox);
    }

}
