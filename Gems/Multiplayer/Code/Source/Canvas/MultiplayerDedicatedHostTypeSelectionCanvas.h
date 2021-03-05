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

namespace Multiplayer
{
    struct MultiplayerDedicatedHostTypeSelectionCanvasContext
    {
        std::function<void()> OnLANButtonClicked;
        std::function<void()> OnGameLiftConnectButtonClicked;
    };

    /*
    * Canvas view to support Multiplayer server hosting type. Currently Supported LAN and GameLift. Handles canvas UI events.
    */
    class MultiplayerDedicatedHostTypeSelectionCanvas
        : public UiCanvasNotificationBus::Handler
    {

    public:
        AZ_CLASS_ALLOCATOR(MultiplayerDedicatedHostTypeSelectionCanvas, AZ::SystemAllocator, 0);
        MultiplayerDedicatedHostTypeSelectionCanvas() {}
        MultiplayerDedicatedHostTypeSelectionCanvas(const MultiplayerDedicatedHostTypeSelectionCanvasContext&);
        virtual ~MultiplayerDedicatedHostTypeSelectionCanvas();

        // UiCanvasActionNotification
        void OnAction(AZ::EntityId entityId, const LyShine::ActionName& actionName) override;

        virtual void Show();
        virtual void Hide();

    private:
        void ShowGameLiftConfig();
        void DismissGameLiftConfig();
        void SaveGameLiftConfig();

        AZ::EntityId m_canvasEntityId;
        MultiplayerDedicatedHostTypeSelectionCanvasContext m_context;

        AZStd::vector< AZStd::string > m_errorMessageQueue;
        bool m_isShowingBusy;
        bool m_isShowingError;
        bool m_isShowingGameLiftConfig;
    };
}
