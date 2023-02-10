/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RPI.Public/ViewportContextBus.h>

#include <Multiplayer/Components/NetworkDebugPlayerIdComponent.h>

#include <Atom/RPI.Public/ViewportContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzNetworking/Framework/INetworking.h>

namespace Multiplayer
{
    void NetworkDebugPlayerIdComponent::Reflect(AZ::ReflectContext* context)
    {
        if (const auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<NetworkDebugPlayerIdComponent, NetworkDebugPlayerIdComponentBase>()
                ->Version(1)
                ->Field("translationOffset", &NetworkDebugPlayerIdComponent::m_translationOffset)
                ->Field("scale", &NetworkDebugPlayerIdComponent::m_fontScale)
                ->Field("color", &NetworkDebugPlayerIdComponent::m_fontColor)
            ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<NetworkDebugPlayerIdComponent>("Network Debug Connection Counter", "Renders the player id as debug text over network players.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "Multiplayer")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                    ->DataElement(AZ::Edit::UIHandlers::Default, &NetworkDebugPlayerIdComponent::m_translationOffset, "Translation Offset", "The world-space offset from the player position to render the debug text.")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &NetworkDebugPlayerIdComponent::m_fontScale, "Font Scale", "Apply a scale to the default debug font rendering size.")
                    ->DataElement(AZ::Edit::UIHandlers::Color, &NetworkDebugPlayerIdComponent::m_fontColor, "Color", "Debug text color.")
                ;
            }
        }

        NetworkDebugPlayerIdComponentBase::Reflect(context);
    }

    void NetworkDebugPlayerIdComponent::OnActivate([[maybe_unused]] Multiplayer::EntityIsMigrating entityIsMigrating)
    {
        AZ::TickBus::Handler::BusConnect();

        const AZ::RPI::ViewportContextPtr viewport = AZ::RPI::ViewportContextRequests::Get()->GetDefaultViewportContext();
        if (!viewport)
        {
            return;
        }

        const auto fontQueryInterface = AZ::Interface<AzFramework::FontQueryInterface>::Get();
        if (!fontQueryInterface)
        {
            return;
        }

        m_fontDrawInterface = fontQueryInterface->GetDefaultFontDrawInterface();
        if (!m_fontDrawInterface)
        {
            return;
        }

        m_drawParams.m_drawViewportId = viewport->GetId();
        m_drawParams.m_scale = AZ::Vector2(m_fontScale);
        m_drawParams.m_hAlign = AzFramework::TextHorizontalAlignment::Center;
        m_drawParams.m_color = m_fontColor;
    }

    void NetworkDebugPlayerIdComponent::OnDeactivate([[maybe_unused]] Multiplayer::EntityIsMigrating entityIsMigrating)
    {
        AZ::TickBus::Handler::BusDisconnect();
    }

    void NetworkDebugPlayerIdComponent::OnTick([[maybe_unused]] float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
    {
        m_drawParams.m_position = GetEntity()->GetTransform()->GetWorldTranslation() + m_translationOffset;

        if (IsNetEntityRoleClient())
        {
            AZStd::string debugText = AZStd::string::format("Player %i", GetPlayerId());
            m_fontDrawInterface->DrawScreenAlignedText3d(m_drawParams, debugText);
        }
        else if (IsNetEntityRoleAutonomous())
        {
            AZStd::string debugText = AZStd::string::format(
                "Player %i of %i", GetPlayerId(), static_cast<NetworkDebugPlayerIdComponentController*>(GetController())->GetConnectionCount());
            m_fontDrawInterface->DrawScreenAlignedText3d(m_drawParams, debugText);
        }
    }

    NetworkDebugPlayerIdComponentController::NetworkDebugPlayerIdComponentController(NetworkDebugPlayerIdComponent& parent)
        : NetworkDebugPlayerIdComponentControllerBase(parent)
    {
    }

    void NetworkDebugPlayerIdComponentController::OnActivate([[maybe_unused]] Multiplayer::EntityIsMigrating entityIsMigrating)
    {
        #if AZ_TRAIT_SERVER
            m_networkInterface = AZ::Interface<AzNetworking::INetworking>::Get()->RetrieveNetworkInterface(AZ::Name(MpNetworkInterfaceName));
            if (IsNetEntityRoleAuthority())
            {
                if (const auto networkInterface =
                        AZ::Interface<AzNetworking::INetworking>::Get()->RetrieveNetworkInterface(AZ::Name(MpNetworkInterfaceName)))
                {
                    uint32_t currentConnectionCount = networkInterface->GetConnectionSet().GetConnectionCount();
                    SetConnectionCount(currentConnectionCount);

                    // Multiplayer system doesn't directly track the number of players.
                    // Instead just assign this player an id by checking how many machines are already connected to this host.
                    // Note: This doesn't support reassigning player-ids to rejoining players
                    // Note: This doesn't consider multi-server connections; it's possible that connection count includes other servers (not just player machines)
                    SetPlayerId(currentConnectionCount); 
                }

                AZ::TickBus::Handler::BusConnect();
            }
        #endif
    }

    void NetworkDebugPlayerIdComponentController::OnDeactivate([[maybe_unused]] Multiplayer::EntityIsMigrating entityIsMigrating)
    {
        #if AZ_TRAIT_SERVER
            if (IsNetEntityRoleAuthority())
            {
                AZ::TickBus::Handler::BusDisconnect();
            }
        #endif
    }

    void NetworkDebugPlayerIdComponentController::OnTick([[maybe_unused]]float deltaTime, [[maybe_unused]]AZ::ScriptTimePoint time)
    {
        if (IsNetEntityRoleAuthority())
        {
            if (m_networkInterface)
            {
                SetConnectionCount(m_networkInterface->GetConnectionSet().GetConnectionCount());
            }            
        }
    }
}
