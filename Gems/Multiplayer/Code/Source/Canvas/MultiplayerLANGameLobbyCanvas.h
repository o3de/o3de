
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

    struct MultiplayerCreateServerViewContext;
    class MultiplayerCreateServerView;
    struct MultiplayerJoinServerViewContext;
    class MultiplayerJoinServerView;

    struct MultiplayerLANGameLobbyCanvasContext
    {
        MultiplayerJoinServerViewContext JoinServerViewContext;
        MultiplayerCreateServerViewContext CreateServerViewContext;
        std::function<void()> OnReturnButtonClicked;
    };

    /*
    * Canvas view to support Multiplayer LAN lobby. Handles canvas UI events.
    */
    class MultiplayerLANGameLobbyCanvas
        : public UiCanvasNotificationBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(MultiplayerLANGameLobbyCanvas, AZ::SystemAllocator, 0);
        MultiplayerLANGameLobbyCanvas() {}
        MultiplayerLANGameLobbyCanvas(const MultiplayerLANGameLobbyCanvasContext&);
        virtual ~MultiplayerLANGameLobbyCanvas();

        // UiCanvasActionNotification
        void OnAction(AZ::EntityId entityId, const LyShine::ActionName& actionName) override;

        virtual void Show();
        virtual void Hide();

        virtual void DisplaySearchResults(const GridMate::GridSearch*);
        virtual void ClearSearchResults();
        virtual int GetSelectedServerResult();
        virtual LyShine::StringType GetMapName() const;
        virtual LyShine::StringType GetServerName() const;
    protected:
        MultiplayerJoinServerView* m_joinServerScreen;
        MultiplayerCreateServerView* m_createServerScreen;

    private:
        AZ::EntityId m_canvasEntityId;
        MultiplayerLANGameLobbyCanvasContext m_context;
    };
}
