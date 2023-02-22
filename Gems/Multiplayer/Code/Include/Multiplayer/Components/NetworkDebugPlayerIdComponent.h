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

namespace AZ::RPI
{
    class ViewportContext;
}

namespace Multiplayer
{
    /*!
     * \class NetworkDebugPlayerIdComponent
     * \brief A component for network players to render their player id in world-space.
     * Both autonomous players and client proxies will have their player ids rendered on screen.
     */
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

        void OnActivate(EntityIsMigrating entityIsMigrating) override;
        void OnDeactivate(EntityIsMigrating entityIsMigrating) override;

        #if AZ_TRAIT_SERVER
            void SetOwningConnectionId(AzNetworking::ConnectionId connectionId) override;
        #endif

        // AZ::TickBus::Handler overrides...
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;

    private:
        // Properties exposed to editor...
        //! \brief The player id will render debug text at the player's world-space location.
        //   This offset is exposed to the editor. It's useful if the player's origin is at the feet, but you wish to render id on the head.
        AZ::Vector3 m_translationOffset = AZ::Vector3(0, 0, 0);

        //! \brief Number exposed to Editor to scale the player id debug text.*/
        float m_fontScale = 0.7f;

        //! \brief Color exposed to Editor to change the player id debug text color.*/
        AZ::Color m_fontColor = AZ::Colors::Wheat;

        // Cache properties required for font rendering...
        AZ::RPI::ViewportContextPtr m_viewport;
        AzFramework::FontDrawInterface* m_fontDrawInterface = nullptr;
        AzFramework::TextDrawParameters m_drawParams;
    };

    class NetworkDebugPlayerIdComponentController
    : public NetworkDebugPlayerIdComponentControllerBase
    {
    public:
        explicit NetworkDebugPlayerIdComponentController(NetworkDebugPlayerIdComponent& parent);

        void OnActivate(EntityIsMigrating entityIsMigrating) override;
        void OnDeactivate(EntityIsMigrating entityIsMigrating) override;

    private:
        void UpdateClientConnectionCount();
        void OnConnectionAcquired();
        void OnEndpointDisconnected();

        AzNetworking::INetworkInterface* m_networkInterface = nullptr;

        ConnectionAcquiredEvent::Handler m_connectionAcquiredHandler = ConnectionAcquiredEvent::Handler(
            [this](MultiplayerAgentDatum)
            {
                this->OnConnectionAcquired();
            });

        EndpointDisconnectedEvent::Handler m_endpointDisconnectedHandler = EndpointDisconnectedEvent::Handler(
            [this](MultiplayerAgentType)
            {
                this->OnEndpointDisconnected();
            });
    };
} // namespace Multiplayer
