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
#include <Multiplayer/MultiplayerEditorConnectionViewportDebugBus.h>

namespace Multiplayer
{
    //! Multiplayer system component wraps the bridging logic between the game and transport layer.
    class MultiplayerEditorConnectionViewportDebugSystemComponent final
        : public AZ::Component
        , private MultiplayerEditorConnectionViewportDebugRequestBus::Handler
        , private AZ::TickBus::Handler
        , private AzToolsFramework::EditorEvents::Bus::Handler
        , private IEditorNotifyListener
    {
    public:
        AZ_COMPONENT(MultiplayerEditorConnectionViewportDebugSystemComponent, "{7600cfcf-e380-4876-aa90-8120e57205e9}");

        static void Reflect(AZ::ReflectContext* context);

        MultiplayerEditorConnectionViewportDebugSystemComponent() = default;
        ~MultiplayerEditorConnectionViewportDebugSystemComponent() override = default;

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

        //! MultiplayerEditorConnectionViewportDebugRequestBus::Handler overrides.
        //! @{
        void DisplayMessage(const char* text) override;
        void StopViewportDebugMessaging() override;
        //! @}

        AZStd::string m_debugText;
    };
}
