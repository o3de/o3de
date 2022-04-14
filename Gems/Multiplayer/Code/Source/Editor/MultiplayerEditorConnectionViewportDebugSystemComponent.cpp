/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "MultiplayerEditorConnectionViewportDebugSystemComponent.h"
#include <Atom/RPI.Public/ViewportContextBus.h>
#include <Atom/RPI.Public/ViewportContext.h>

namespace Multiplayer
{
    void MultiplayerEditorConnectionViewportDebugSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<MultiplayerEditorConnectionViewportDebugSystemComponent, AZ::Component>()
                ->Version(1);
        }
    }

    void MultiplayerEditorConnectionViewportDebugSystemComponent::Activate()
    {
        AzToolsFramework::EditorEvents::Bus::Handler::BusConnect();
        MultiplayerEditorConnectionViewportDebugRequestBus::Handler::BusConnect();
    }

    void MultiplayerEditorConnectionViewportDebugSystemComponent::Deactivate()
    {
        AzToolsFramework::EditorEvents::Bus::Handler::BusDisconnect();
        MultiplayerEditorConnectionViewportDebugRequestBus::Handler::BusDisconnect();
    }

    void MultiplayerEditorConnectionViewportDebugSystemComponent::NotifyRegisterViews()
    {
        IEditor* editor = nullptr;
        AzToolsFramework::EditorRequests::Bus::BroadcastResult(editor, &AzToolsFramework::EditorRequests::GetEditor);
        if (editor)
        {
            editor->RegisterNotifyListener(this);
        }
    }

    void MultiplayerEditorConnectionViewportDebugSystemComponent::OnTick(float, AZ::ScriptTimePoint)
    {
        if (!m_debugText.empty())
        {            
            const AZ::RPI::ViewportContextPtr viewport = AZ::Interface<AZ::RPI::ViewportContextRequestsInterface>::Get()->GetDefaultViewportContext();
            AzFramework::WindowSize viewportSize = viewport->GetViewportSize();
            const int center_screenposition_x = viewportSize.m_width/2;
            const int center_screenposition_y = viewportSize.m_height/2;
            const float fontsize = 0.7f;
            const int screenposition_title_y = center_screenposition_y-9;
            const int screenposition_debugtext_y = screenposition_title_y+18;

            AzFramework::DebugDisplayRequestBus::Broadcast(&AzFramework::DebugDisplayRequestBus::Events::SetColor, AZ::Colors::Yellow);
            AzFramework::DebugDisplayRequestBus::Broadcast(&AzFramework::DebugDisplayRequestBus::Events::Draw2dTextLabel, 
                center_screenposition_x, screenposition_title_y, fontsize, "Multiplayer Editor", true);

            AzFramework::DebugDisplayRequestBus::Broadcast(&AzFramework::DebugDisplayRequestBus::Events::SetColor, AZ::Colors::White);
            AzFramework::DebugDisplayRequestBus::Broadcast(&AzFramework::DebugDisplayRequestBus::Events::Draw2dTextLabel, 
                center_screenposition_x, screenposition_debugtext_y, fontsize, m_debugText.c_str(), true);
        }
    }

    void MultiplayerEditorConnectionViewportDebugSystemComponent::DisplayMessage(const char* text)
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
    
    void MultiplayerEditorConnectionViewportDebugSystemComponent::StopViewportDebugMessaging()
    {
        m_debugText = "";
        AZ::TickBus::Handler::BusDisconnect();
    }

    void MultiplayerEditorConnectionViewportDebugSystemComponent::OnEditorNotifyEvent(EEditorNotifyEvent event)
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
