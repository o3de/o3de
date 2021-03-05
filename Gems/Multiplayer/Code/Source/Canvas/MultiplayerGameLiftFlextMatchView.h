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
    struct MultiplayerGameLiftFlextMatchViewContext
    {
        AZStd::string DefaultMatchmakingConfig;
        std::function<void()> OnStartMatchmakingButtonClicked;
    };

    /*
    * View to support GameLift flex match. Handles canvas UI events.
    */
    class MultiplayerGameLiftFlextMatchView : public UiCanvasNotificationBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(MultiplayerGameLiftFlextMatchView, AZ::SystemAllocator, 0);
        MultiplayerGameLiftFlextMatchView(const MultiplayerGameLiftFlextMatchViewContext&, const AZ::EntityId);
        virtual ~MultiplayerGameLiftFlextMatchView();

        // UiCanvasActionNotification
        void OnAction(AZ::EntityId entityId, const LyShine::ActionName& actionName) override;

    private:
        void SaveMatchmakingConfigName();
        AZ::EntityId m_canvasEntityId;
        MultiplayerGameLiftFlextMatchViewContext m_context;
    };
}
