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
    struct MultiplayerCreateServerViewContext
    {
        AZStd::string DefaultMapName;
        AZStd::string DefaultServerName;
        std::function<void()> OnCreateServerButtonClicked;
    };

    /*
    * View to support Multiplayer Create Server. Handles UI events for creating server.
    */
    class MultiplayerCreateServerView
        : public UiCanvasNotificationBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(MultiplayerCreateServerView, AZ::SystemAllocator, 0);
        MultiplayerCreateServerView(const MultiplayerCreateServerViewContext&, const AZ::EntityId);
        virtual ~MultiplayerCreateServerView();

        // UiCanvasActionNotification
        void OnAction(AZ::EntityId entityId, const LyShine::ActionName& actionName) override;

        LyShine::StringType GetMapName() const;
        LyShine::StringType GetServerName() const;

    private:
        AZ::EntityId m_canvasEntityId;
        MultiplayerCreateServerViewContext m_context;
    };
}
