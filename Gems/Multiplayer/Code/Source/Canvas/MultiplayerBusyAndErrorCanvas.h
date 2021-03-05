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
    struct MultiplayerBusyAndErrorCanvasContext
    {
        std::function<void(bool)> OnDismissErrroWindowButtonClicked;
    };

    /*
    * Canvas to support Multiplayer busy and error screens. Handles canvas UI events. Load last to overlay over others
    */
    class MultiplayerBusyAndErrorCanvas
        : public UiCanvasNotificationBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(MultiplayerBusyAndErrorCanvas, AZ::SystemAllocator, 0);
        MultiplayerBusyAndErrorCanvas() {}
        MultiplayerBusyAndErrorCanvas(const MultiplayerBusyAndErrorCanvasContext&);
        virtual ~MultiplayerBusyAndErrorCanvas();

        // UiCanvasActionNotification
        void OnAction(AZ::EntityId entityId, const LyShine::ActionName& actionName) override;

        virtual void ShowError(const char* error);
        virtual void DismissError(bool force = false);

        virtual void ShowBusyScreen();
        virtual void DismissBusyScreen(bool force = false);

    private:
        void ShowQueuedErrorMessage();

        AZ::EntityId m_canvasEntityId;
        MultiplayerBusyAndErrorCanvasContext m_context;
        AZStd::vector< AZStd::string > m_errorMessageQueue;
        bool m_isShowingError;
        bool m_isShowingBusy;
    };
}
