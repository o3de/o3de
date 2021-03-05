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
#include <Source/Canvas/MultiplayerJoinServerView.h>
#include <Source/Canvas/MultiplayerCreateServerView.h>
#include <Source/Canvas/MultiplayerGameLiftFlextMatchView.h>

namespace Multiplayer
{
    struct MultiplayerGameLiftLobbyCanvasContext
    {
        MultiplayerJoinServerViewContext JoinServerViewContext;
        MultiplayerGameLiftFlextMatchViewContext GameLiftFlexMatchViewContext;
        MultiplayerCreateServerViewContext CreateServerViewContext;
        std::function<void()> OnReturnButtonClicked;
    };

    /*
    * Canvas view to support GameLift lobby. Handles canvas UI events.
    */
    class MultiplayerGameLiftLobbyCanvas
        : public UiCanvasNotificationBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(MultiplayerGameLiftLobbyCanvas, AZ::SystemAllocator, 0);
        MultiplayerGameLiftLobbyCanvas() {}
        MultiplayerGameLiftLobbyCanvas(const MultiplayerGameLiftLobbyCanvasContext&);
        virtual ~MultiplayerGameLiftLobbyCanvas();

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
        MultiplayerGameLiftFlextMatchView* m_flexMatchScreen;
        MultiplayerCreateServerView* m_createServerScreen;

    private:
        void SaveGameLiftConfig();
        void RefreshGameLiftConfig();
        AZ::EntityId m_canvasEntityId;
        MultiplayerGameLiftLobbyCanvasContext m_context;
    };
}
