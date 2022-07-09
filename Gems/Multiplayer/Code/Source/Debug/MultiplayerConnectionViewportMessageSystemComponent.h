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
#include <AzFramework/Font/FontInterface.h>
#include <Multiplayer/IMultiplayer.h>
#include <Multiplayer/IMultiplayerConnectionViewportMessage.h>

namespace Multiplayer
{
    //! System component that draws viewport messaging as the editor attempts connection to the editor-server while starting up game-mode.
    class MultiplayerConnectionViewportMessageSystemComponent final
        : public AZ::Component
        , public IMultiplayerConnectionViewportMessage
        , private AZ::TickBus::Handler
    {
    public:
        AZ_COMPONENT(MultiplayerConnectionViewportMessageSystemComponent, "{7600cfcf-e380-4876-aa90-8120e57205e9}", IMultiplayerConnectionViewportMessage);

        static void Reflect(AZ::ReflectContext* context);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);

        MultiplayerConnectionViewportMessageSystemComponent();
        ~MultiplayerConnectionViewportMessageSystemComponent() override;

        //! AZ::Component overrides.
        //! @{
        void Activate() override;
        void Deactivate() override;
        //! @}


    private:
        //! AZ::TickBus::Handler
        //! @{
        void OnTick(float, AZ::ScriptTimePoint) override;
        //! @}

        //! IMultiplayerConnectionViewportMessage overrides.
        //! @{
        void DisplayCenterViewportMessage(const char* text) override;
        void StopCenterViewportDebugMessaging() override;
        //! @}

        void DrawConnectionStatus(AzNetworking::ConnectionState connectionState);

        AZStd::string m_centerViewportDebugText;
        AzFramework::FontDrawInterface* m_fontDrawInterface = nullptr;
        AzFramework::TextDrawParameters m_drawParams;
        float m_lineSpacing = 0.0f;
    };
}
