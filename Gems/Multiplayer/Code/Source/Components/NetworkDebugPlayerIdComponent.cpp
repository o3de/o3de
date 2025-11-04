/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Multiplayer/Components/NetworkDebugPlayerIdComponent.h>
#include <Multiplayer/ConnectionData/IConnectionData.h>

#include <Atom/RPI.Public/ViewportContextBus.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzFramework/Viewport/ViewportScreen.h>
#include <AzNetworking/Framework/INetworking.h>

namespace Multiplayer
{
    AZ_CVAR(bool, cl_debugPlayerIdRender, true, nullptr, AZ::ConsoleFunctorFlags::DontReplicate, "Whether NetworkDebugPlayerIdComponent should render the player id and connection count on-screen.");

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
                editContext->Class<NetworkDebugPlayerIdComponent>("Network Debug Player ID", "Renders the player id as debug text over network players.")
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

    void NetworkDebugPlayerIdComponent::OnActivate([[maybe_unused]] EntityIsMigrating entityIsMigrating)
    {
        #if (AZ_TRAIT_CLIENT)
            m_viewport = AZ::RPI::ViewportContextRequests::Get()->GetDefaultViewportContext();
            if (!m_viewport)
            {
                AZ_Assert(false, "NetworkDebugPlayerIdComponent failed to find the a rendering viewport. Debug rendering will be disabled.");
                return;
            }

            const auto fontQueryInterface = AZ::Interface<AzFramework::FontQueryInterface>::Get();
            if (!fontQueryInterface)
            {
                AZ_Assert(false, "NetworkDebugPlayerIdComponent failed to find the FontQueryInterface. Debug rendering will be disabled.");
                return;
            }

            m_fontDrawInterface = fontQueryInterface->GetDefaultFontDrawInterface();
            if (!m_fontDrawInterface)
            {
                AZ_Assert(false, "NetworkDebugPlayerIdComponent failed to find the FontDrawInterface. Debug rendering will be disabled.");
                return;
            }

            m_drawParams.m_drawViewportId = m_viewport->GetId();
            m_drawParams.m_scale = AZ::Vector2(m_fontScale);
            m_drawParams.m_color = m_fontColor;

            AZ::TickBus::Handler::BusConnect();
        #endif
    }

    void NetworkDebugPlayerIdComponent::OnDeactivate([[maybe_unused]] EntityIsMigrating entityIsMigrating)
    {
        AZ::TickBus::Handler::BusDisconnect();
    }

    #if AZ_TRAIT_SERVER
    void NetworkDebugPlayerIdComponent::SetOwningConnectionId(AzNetworking::ConnectionId connectionId)
    {
        if (IsNetEntityRoleAuthority())
        {
            const auto controller = static_cast<NetworkDebugPlayerIdComponentController*>(GetController());
            controller->SetPlayerId(connectionId);
        }
    }
    #endif

    void NetworkDebugPlayerIdComponent::OnTick([[maybe_unused]] float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
    {
        if (!cl_debugPlayerIdRender)
        {
            return;
        }

        // Don't render others players' on-screen debug text if the player is behind the camera
        AZ::Vector3 renderWorldSpace = GetEntity()->GetTransform()->GetWorldTranslation() + m_translationOffset;
        if (!IsNetEntityRoleAutonomous())
        {
            AZ::Vector3 cameraForward = m_viewport->GetCameraTransform().GetBasisY();
            AZ::Vector3 cameraToPlayer = renderWorldSpace - m_viewport->GetCameraTransform().GetTranslation();
            if (cameraForward.Dot(cameraToPlayer) < 0.0f)
            {
                return;
            }   
        }

        const AzFramework::WindowSize windowSize = m_viewport->GetViewportSize();
        AzFramework::ScreenPoint renderScreenpoint = AzFramework::WorldToScreen(
            renderWorldSpace, m_viewport->GetCameraViewMatrixAsMatrix3x4(), m_viewport->GetCameraProjectionMatrix(), AzFramework::ScreenSize(windowSize.m_width, windowSize.m_height));
        m_drawParams.m_hAlign = AzFramework::TextHorizontalAlignment::Center;
        m_drawParams.m_position = AZ::Vector3(aznumeric_cast<float>(renderScreenpoint.m_x), aznumeric_cast<float>(renderScreenpoint.m_y), 0.f);

        AZStd::string playerIdText = AZStd::string::format("Player %i", static_cast<uint32_t>(GetPlayerId()));
        m_fontDrawInterface->DrawScreenAlignedText2d(m_drawParams, playerIdText);

        if (IsNetEntityRoleAutonomous())
        {
            // Render connection count in the lower right-hand corner
            const float textHeight = m_fontDrawInterface->GetTextSize(m_drawParams, playerIdText).GetY();
            const float lineSpacing = 0.5f * textHeight;
            const AzFramework::WindowSize viewportSize = m_viewport->GetViewportSize();
            const AZ::Vector3 viewportConnectionBottomRightBorderPadding(-40.0f, -40.0f + textHeight + lineSpacing, 0.0f);
            m_drawParams.m_hAlign = AzFramework::TextHorizontalAlignment::Right;
            m_drawParams.m_position =
                AZ::Vector3(aznumeric_cast<float>(viewportSize.m_width), aznumeric_cast<float>(viewportSize.m_height), 1.0f) +
                AZ::Vector3(viewportConnectionBottomRightBorderPadding) * m_viewport->GetDpiScalingFactor();

            const auto controller = static_cast<NetworkDebugPlayerIdComponentController*>(GetController());
            AZStd::string playerCountText = AZStd::string::format("Player Count: %i", controller->GetConnectionCount());
            m_fontDrawInterface->DrawScreenAlignedText2d(m_drawParams, playerCountText);
        }
    }

    NetworkDebugPlayerIdComponentController::NetworkDebugPlayerIdComponentController(NetworkDebugPlayerIdComponent& parent)
        : NetworkDebugPlayerIdComponentControllerBase(parent)
    {
    }

    void NetworkDebugPlayerIdComponentController::OnActivate([[maybe_unused]] EntityIsMigrating entityIsMigrating)
    {
        # if AZ_TRAIT_SERVER
            if (IsNetEntityRoleAuthority())
            {
                m_networkInterface = AZ::Interface<AzNetworking::INetworking>::Get()->RetrieveNetworkInterface(AZ::Name(MpNetworkInterfaceName));
                UpdateClientConnectionCount();
                AZ::Interface<IMultiplayer>::Get()->AddConnectionAcquiredHandler(m_connectionAcquiredHandler);
                AZ::Interface<IMultiplayer>::Get()->AddEndpointDisconnectedHandler(m_endpointDisconnectedHandler);
            }
        #endif
    }

    void NetworkDebugPlayerIdComponentController::OnDeactivate([[maybe_unused]] EntityIsMigrating entityIsMigrating)
    {
        m_connectionAcquiredHandler.Disconnect();
        m_endpointDisconnectedHandler.Disconnect();
    }

    void NetworkDebugPlayerIdComponentController::UpdateClientConnectionCount()
    {
        if (IsNetEntityRoleAuthority())
        {
            uint32_t clientConnectionCount = 0;
            auto sendNetworkUpdates = [&clientConnectionCount](AzNetworking::IConnection& connection)
            {
                if (connection.GetUserData() != nullptr)
                {
                    const auto connectionData = reinterpret_cast<IConnectionData*>(connection.GetUserData());
                    if (connectionData->GetConnectionDataType() == ConnectionDataType::ServerToClient)
                    {
                        clientConnectionCount++;
                    }
                }
            };

            m_networkInterface->GetConnectionSet().VisitConnections(sendNetworkUpdates);
            SetConnectionCount(clientConnectionCount);
        }
    }

    void NetworkDebugPlayerIdComponentController::OnConnectionAcquired()
    {
        uint32_t currentConnectionCount = m_networkInterface->GetConnectionSet().GetConnectionCount() + 1;
        SetConnectionCount(currentConnectionCount);
    }

    void NetworkDebugPlayerIdComponentController::OnEndpointDisconnected()
    {
        uint32_t currentConnectionCount = m_networkInterface->GetConnectionSet().GetConnectionCount() - 1;
        SetConnectionCount(currentConnectionCount);
    }
} // namespace Multiplayer
