/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Source/AutoGen/NetworkDebugPlayerIdComponent.AutoComponent.h>
#include <Atom/RPI.Public/ViewportContext.h>
#include <AzCore/Component/TickBus.h>
#include <AzFramework/Font/FontInterface.h>

namespace AZ
{
    namespace RPI
    {
        class ViewportContext;
    }
}

namespace Multiplayer
{
    class NetworkDebugPlayerIdComponent
    : public NetworkDebugPlayerIdComponentBase
    , private AZ::TickBus::Handler
    {
    public:
        AZ_MULTIPLAYER_COMPONENT(
            Multiplayer::NetworkDebugPlayerIdComponent,
            s_networkDebugPlayerIdComponentConcreteUuid,
            Multiplayer::NetworkDebugPlayerIdComponentBase);

        static void Reflect(AZ::ReflectContext* context);

        void OnActivate(Multiplayer::EntityIsMigrating entityIsMigrating) override;
        void OnDeactivate(Multiplayer::EntityIsMigrating entityIsMigrating) override;

        // AZ::TickBus::Handler overrides...
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;

    private:
        // Properties exposed to editor
        AZ::Vector3 m_translationOffset = AZ::Vector3(0, 0, 0);
        float m_fontScale = 0.7f;
        AZ::Color m_fontColor = AZ::Colors::Wheat;

        // Cache properties required for font rendering
        AZ::RPI::ViewportContextPtr m_viewport;
        AzFramework::FontDrawInterface* m_fontDrawInterface = nullptr;
        AzFramework::TextDrawParameters m_drawParams;
    };

    class NetworkDebugPlayerIdComponentController
    : public NetworkDebugPlayerIdComponentControllerBase
    , private AZ::TickBus::Handler
    {
    public:
        explicit NetworkDebugPlayerIdComponentController(NetworkDebugPlayerIdComponent& parent);

        void OnActivate(EntityIsMigrating entityIsMigrating) override;
        void OnDeactivate(EntityIsMigrating entityIsMigrating) override;

    private:
        // AZ::TickBus::Handler overrides...
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;

        AzNetworking::INetworkInterface* m_networkInterface = nullptr;
    };
}
