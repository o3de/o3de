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
#pragma once

#include <LyShine/Bus/UiCanvasBus.h>
#include <GridMate/GridMate.h>

namespace Multiplayer
{
    /*
    * Stores find server listing data
    */
    struct ServerListingResultRowData
    {
        int RowElementId;
        int TextElementId;
        int HighlightElementId;

        ServerListingResultRowData(int row, int text, int highlight)
        {
            this->RowElementId = row;
            this->TextElementId = text;
            this->HighlightElementId = highlight;
        }
    };

    struct MultiplayerJoinServerViewContext
    {
        std::function<void()> OnJoinButtonClicked;
        std::function<void()> OnRefreshButtonClicked;
        AZStd::vector<ServerListingResultRowData> ServerListingVector;
    };

    /*
    * View to support Multiplayer find and join Server. Handles canvas UI events.
    */
    class MultiplayerJoinServerView
        : public UiCanvasNotificationBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(MultiplayerJoinServerView, AZ::SystemAllocator, 0);
        MultiplayerJoinServerView(const MultiplayerJoinServerViewContext&, const AZ::EntityId);
        virtual ~MultiplayerJoinServerView();

        // UiCanvasActionNotification
        void OnAction(AZ::EntityId entityId, const LyShine::ActionName& actionName) override;

        void DisplaySearchResults(const GridMate::GridSearch*);
        void ClearSearchResults();
        void SelectId(int rowId);
        int m_selectedServerResult;

    private:
        AZ::EntityId m_canvasEntityId;
        MultiplayerJoinServerViewContext m_context;

        /*
        * Defines UI view and actions for listing servers
        */
        class ServerListingResultRow
        {
        public:
            ServerListingResultRow(const AZ::EntityId& canvas, int row, int text, int highlight);

            int GetRowID();

            void Select();
            void Deselect();

            void DisplayResult(const GridMate::SearchInfo* result);

            void ResetDisplay();

        private:

            AZ::EntityId m_canvas;
            int m_row;
            int m_text;
            int m_highlight;
        };

        AZStd::vector< ServerListingResultRow > m_listingRows;
    };
}
