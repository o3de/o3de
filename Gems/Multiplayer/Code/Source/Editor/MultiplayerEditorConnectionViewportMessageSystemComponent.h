/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TickBus.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <IEditor.h>
#include <Multiplayer/IMultiplayerEditorConnectionViewportMessage.h>

namespace Multiplayer
{
    //! System component that draws viewport messaging as the editor attempts connection to the editor-server while starting up game-mode.
    class MultiplayerEditorConnectionViewportMessageSystemComponent final
        : public AZ::Component
        , public IMultiplayerEditorConnectionViewportMessage
        , private AZ::TickBus::Handler
        , private AzToolsFramework::EditorEvents::Bus::Handler
        , private IEditorNotifyListener
    {
    public:
        AZ_COMPONENT(MultiplayerEditorConnectionViewportMessageSystemComponent, "{7600cfcf-e380-4876-aa90-8120e57205e9}", IMultiplayerEditorConnectionViewportMessage);

        static void Reflect(AZ::ReflectContext* context);

        MultiplayerEditorConnectionViewportMessageSystemComponent();
        ~MultiplayerEditorConnectionViewportMessageSystemComponent() override;

        //! AZ::Component overrides.
        //! @{
        void Activate() override;
        void Deactivate() override;
        //! @}

        //! EditorEvents::Bus::Handler overrides.
        //! @{
        void NotifyRegisterViews() override;
        //! @}

    private:
        //! EditorEvents::Handler overrides
        //! @{
        void OnEditorNotifyEvent(EEditorNotifyEvent event) override;
        //! @}

        //! AZ::TickBus::Handler
        //! @{
        void OnTick(float, AZ::ScriptTimePoint) override;
        //! @}

        //! IMultiplayerEditorConnectionViewportMessage overrides.
        //! @{
        void DisplayMessage(const char* text) override;
        void StopViewportDebugMessaging() override;
        //! @}

        AZStd::string m_debugText;
    };
}
