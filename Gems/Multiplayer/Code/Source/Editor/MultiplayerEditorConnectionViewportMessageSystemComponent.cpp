/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "MultiplayerEditorConnectionViewportMessageSystemComponent.h"
#include <Atom/RPI.Public/ViewportContextBus.h>
#include <Atom/RPI.Public/ViewportContext.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <AzCore/Console/IConsole.h>

namespace Multiplayer
{
    const float defaultConnectionMessageFontSize = 0.7f;
    AZ_CVAR_SCOPED(float, editorsv_connectionMessageFontSize, defaultConnectionMessageFontSize, nullptr, AZ::ConsoleFunctorFlags::Null, 
        "The font size used for displaying updates on screen while the multiplayer editor is connecting to the server.");

    void MultiplayerEditorConnectionViewportMessageSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<MultiplayerEditorConnectionViewportMessageSystemComponent, AZ::Component>()
                ->Version(1);
        }
    }
    
    MultiplayerEditorConnectionViewportMessageSystemComponent::MultiplayerEditorConnectionViewportMessageSystemComponent()
    {
        AZ::Interface<IMultiplayerEditorConnectionViewportMessage>::Register(this);
    }

    MultiplayerEditorConnectionViewportMessageSystemComponent::~MultiplayerEditorConnectionViewportMessageSystemComponent()
    {
        AZ::Interface<IMultiplayerEditorConnectionViewportMessage>::Unregister(this);
    };

    void MultiplayerEditorConnectionViewportMessageSystemComponent::Activate()
    {
        AzToolsFramework::EditorEvents::Bus::Handler::BusConnect();
    }

    void MultiplayerEditorConnectionViewportMessageSystemComponent::Deactivate()
    {
        AzToolsFramework::EditorEvents::Bus::Handler::BusDisconnect();
    }

    void MultiplayerEditorConnectionViewportMessageSystemComponent::NotifyRegisterViews()
    {
        IEditor* editor = nullptr;
        AzToolsFramework::EditorRequests::Bus::BroadcastResult(editor, &AzToolsFramework::EditorRequests::GetEditor);
        if (editor)
        {
            editor->RegisterNotifyListener(this);
        }
    }

    void MultiplayerEditorConnectionViewportMessageSystemComponent::OnTick(float, AZ::ScriptTimePoint)
    {
        if (!m_debugText.empty())
        {            
            const AZ::RPI::ViewportContextPtr viewport = AZ::Interface<AZ::RPI::ViewportContextRequestsInterface>::Get()->GetDefaultViewportContext();
            AzFramework::WindowSize viewportSize = viewport->GetViewportSize();
            const float center_screenposition_x = 0.5f*viewportSize.m_width;
            const float center_screenposition_y = 0.5f*viewportSize.m_height;
            const float screenposition_title_y = center_screenposition_y-9;
            const float screenposition_debugtext_y = screenposition_title_y+18;

            AzFramework::DebugDisplayRequestBus::Broadcast(&AzFramework::DebugDisplayRequestBus::Events::SetColor, AZ::Colors::Yellow);
            AzFramework::DebugDisplayRequestBus::Broadcast(&AzFramework::DebugDisplayRequestBus::Events::Draw2dTextLabel, 
                center_screenposition_x, screenposition_title_y, editorsv_connectionMessageFontSize, "Multiplayer Editor", true);

            AzFramework::DebugDisplayRequestBus::Broadcast(&AzFramework::DebugDisplayRequestBus::Events::SetColor, AZ::Colors::White);
            AzFramework::DebugDisplayRequestBus::Broadcast(&AzFramework::DebugDisplayRequestBus::Events::Draw2dTextLabel, 
                center_screenposition_x, screenposition_debugtext_y, editorsv_connectionMessageFontSize, m_debugText.c_str(), true);
        }
    }

    void MultiplayerEditorConnectionViewportMessageSystemComponent::DisplayMessage(const char* text)
    {
        if (strlen(text) == 0)
        {
            StopViewportDebugMessaging();
            return;
        }
        
        m_debugText = text;
        if (!AZ::TickBus::Handler::BusIsConnected())
        {
            AZ::TickBus::Handler::BusConnect();
        }
    }
    
    void MultiplayerEditorConnectionViewportMessageSystemComponent::StopViewportDebugMessaging()
    {
        m_debugText = "";
        AZ::TickBus::Handler::BusDisconnect();
    }

    void MultiplayerEditorConnectionViewportMessageSystemComponent::OnEditorNotifyEvent(EEditorNotifyEvent event)
    {
        switch (event)
        {
        // If the user exits game mode before connection is finished then stop showing messages in the viewport.
        case eNotify_OnQuit:
            [[fallthrough]];
        case eNotify_OnEndGameMode:
            StopViewportDebugMessaging();
            break;
        }
    }
}
