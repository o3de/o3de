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
#include <GridMate/Session/Session.h>
#include <LyShine/Bus/UiElementBus.h>
#include <LyShine/Bus/UiTextBus.h>
#include <Source/Canvas/MultiplayerJoinServerView.h>
#include <Source/Canvas/MultiplayerCanvasHelper.h>

namespace Multiplayer
{
    static const char* JoinButton = "JoinButton";

    MultiplayerJoinServerView::MultiplayerJoinServerView(const MultiplayerJoinServerViewContext& context, const AZ::EntityId canvasEntityId)
        : m_context(context), m_canvasEntityId(canvasEntityId)
    {
        AZ_Error("MultiplayerLobbyComponent", m_canvasEntityId.IsValid(), "Invalid CanvasId passed in");
        for (ServerListingResultRowData serverListingData : m_context.ServerListingVector)
        {
            m_listingRows.emplace_back(m_canvasEntityId, serverListingData.RowElementId, serverListingData.TextElementId, serverListingData.HighlightElementId);
        }
        
        UiCanvasNotificationBus::Handler::BusConnect(m_canvasEntityId);
        ClearSearchResults();
    }

    MultiplayerJoinServerView::~MultiplayerJoinServerView()
    {
        UiCanvasNotificationBus::Handler::BusDisconnect(m_canvasEntityId);
    }

    void MultiplayerJoinServerView::OnAction(AZ::EntityId entityId, const LyShine::ActionName& actionName)
    {
        AZ_UNUSED(entityId)
        if (actionName == "OnJoinServer")
        {
            m_context.OnJoinButtonClicked();
        }
        else if (actionName == "OnRefresh")
        {
            m_context.OnRefreshButtonClicked();
        }
        else if (actionName == "OnSelectServer")
        {
            LyShine::ElementId elementId = 0;
            EBUS_EVENT_ID_RESULT(elementId, entityId, UiElementBus, GetElementId);

            SelectId(elementId);
        }
    }

    void MultiplayerJoinServerView::DisplaySearchResults(const GridMate::GridSearch* search)
    {
        for (unsigned int i = 0; i < search->GetNumResults(); ++i)
        {
            // Screen is currently not dynamically populated, so we are stuck with a fixed size
            // amount of results for now.
            if (i >= m_listingRows.size())
            {
                break;
            }

            const GridMate::SearchInfo* searchInfo = search->GetResult(i);

            ServerListingResultRow& resultRow = m_listingRows[i];

            resultRow.DisplayResult(searchInfo);
        }
    }

    void MultiplayerJoinServerView::ClearSearchResults()
    {
        m_selectedServerResult = -1;

        for (ServerListingResultRow& serverResultRow : m_listingRows)
        {
            serverResultRow.ResetDisplay();
        }

        SetElementInputEnabled(m_canvasEntityId, JoinButton, false);
    }

    void MultiplayerJoinServerView::SelectId(int rowId)
    {
        SetElementInputEnabled(m_canvasEntityId, JoinButton, false);
        int lastSelection = m_selectedServerResult;

        m_selectedServerResult = -1;
        for (int i = 0; i < static_cast<int>(m_listingRows.size()); ++i)
        {
            ServerListingResultRow& resultRow = m_listingRows[i];

            if (resultRow.GetRowID() == rowId)
            {
               
                SetElementInputEnabled(m_canvasEntityId, JoinButton, true);
                m_selectedServerResult = i;
                resultRow.Select();
            }
            else
            {
                resultRow.Deselect();
            }
        }

        // Double click to join.
        if (m_selectedServerResult >= 0 && lastSelection == m_selectedServerResult)
        {
            m_context.OnJoinButtonClicked();
        }
    }


    ///////////////////////////
    // ServerListingResultRow
    ///////////////////////////

    MultiplayerJoinServerView::ServerListingResultRow::ServerListingResultRow(const AZ::EntityId& canvas, int row, int text, int highlight)
        : m_canvas(canvas)
        , m_row(row)
        , m_text(text)
        , m_highlight(highlight)
    {
    }

    int MultiplayerJoinServerView::ServerListingResultRow::GetRowID()
    {
        return m_row;
    }

    void MultiplayerJoinServerView::ServerListingResultRow::Select()
    {
        AZ::Entity* element = nullptr;
        EBUS_EVENT_ID_RESULT(element, m_canvas, UiCanvasBus, FindElementById, m_highlight);

        if (element != nullptr)
        {
            EBUS_EVENT_ID(element->GetId(), UiElementBus, SetIsEnabled, true);
        }
    }

    void MultiplayerJoinServerView::ServerListingResultRow::Deselect()
    {
        AZ::Entity* element = nullptr;
        EBUS_EVENT_ID_RESULT(element, m_canvas, UiCanvasBus, FindElementById, m_highlight);

        if (element != nullptr)
        {
            EBUS_EVENT_ID(element->GetId(), UiElementBus, SetIsEnabled, false);
        }
    }

    void MultiplayerJoinServerView::ServerListingResultRow::DisplayResult(const GridMate::SearchInfo* searchInfo)
    {
        char displayString[64];

        const char* serverName = "";

        for (unsigned int i = 0; i < searchInfo->m_numParams; ++i)
        {
            const GridMate::GridSessionParam& param = searchInfo->m_params[i];

            if (param.m_id == "sv_name")
            {
                serverName = param.m_value.c_str();
                break;
            }
        }

        azsnprintf(displayString, AZ_ARRAY_SIZE(displayString), "%s (%u/%u)", serverName, searchInfo->m_numUsedPublicSlots, searchInfo->m_numFreePublicSlots + searchInfo->m_numUsedPublicSlots);

        AZ::Entity* element = nullptr;

        EBUS_EVENT_ID_RESULT(element, m_canvas, UiCanvasBus, FindElementById, m_text);

        if (element != nullptr)
        {
            LyShine::StringType textString(displayString);
            EBUS_EVENT_ID(element->GetId(), UiTextBus, SetText, textString);
        }
    }

    void MultiplayerJoinServerView::ServerListingResultRow::ResetDisplay()
    {
        Deselect();
    }
}
