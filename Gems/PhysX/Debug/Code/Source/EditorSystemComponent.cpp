/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorSystemComponent.h"
#include <AzCore/Interface/Interface.h>
#include <AzFramework/Physics/PhysicsSystem.h>
#include <AzFramework/Physics/SystemBus.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>

#include <PhysX/Debug/PhysXDebugInterface.h>
#include <PhysX/Debug/PhysXDebugConfiguration.h>
#include <PhysX/Configuration/PhysXConfiguration.h>

#include <IEditor.h>

namespace PhysXDebug
{
    void EditorSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorSystemComponent, AZ::Component>()
                ->Version(1)
                ;
        }
    }

    // This will be called when the IEditor instance is ready
    void EditorSystemComponent::NotifyRegisterViews()
    {
        RegisterForEditorEvents();
    }

    void EditorSystemComponent::OnCrySystemShutdown(ISystem&)
    {
        UnregisterForEditorEvents();
    }

    void EditorSystemComponent::OnEditorNotifyEvent(const EEditorNotifyEvent editorEvent)
    {
        switch (editorEvent)
        {
        case eNotify_OnEndNewScene:
        case eNotify_OnEndLoad:
            AutoConnectPVD();
            break;
        default:
            // Intentionally left blank.
            break;
        }
    }

    void EditorSystemComponent::Activate()
    {
        AzToolsFramework::EditorEvents::Bus::Handler::BusConnect();
        AzToolsFramework::Components::EditorComponentBase::Activate();
        AzToolsFramework::EditorEntityContextNotificationBus::Handler::BusConnect();
        CrySystemEventBus::Handler::BusConnect();

        if (auto* physXDebug = AZ::Interface<PhysX::Debug::PhysXDebugInterface>::Get())
        {
            m_pvdConfigurationChangeHandler = PhysX::Debug::PvdConfigurationChangedEvent::Handler(
                [this](const PhysX::Debug::PvdConfiguration& config)
                {
                    this->OnPvdConfigurationChanged(config);
                });
            m_colliderProximityVisualizationChangedEventHandler = PhysX::Debug::ColliderProximityVisualizationChangedEvent::Handler(
                [this](const PhysX::Debug::ColliderProximityVisualization& visualizationData)
                {
                    this->OnColliderProximityVisualizationChanged(visualizationData);
                });

            physXDebug->RegisterPvdConfigurationChangedEvent(m_pvdConfigurationChangeHandler);
            physXDebug->RegisterColliderProximityVisualizationChangedEvent(m_colliderProximityVisualizationChangedEventHandler);
        }
    }

    void EditorSystemComponent::Deactivate()
    {
        m_pvdConfigurationChangeHandler.Disconnect();
        m_colliderProximityVisualizationChangedEventHandler.Disconnect();

        CrySystemEventBus::Handler::BusDisconnect();
        AzToolsFramework::EditorEntityContextNotificationBus::Handler::BusDisconnect();
        AzToolsFramework::Components::EditorComponentBase::Deactivate();
        AzToolsFramework::EditorEvents::Bus::Handler::BusDisconnect();
    }

    void EditorSystemComponent::RegisterForEditorEvents()
    {
        IEditor* editor = nullptr;
        AzToolsFramework::EditorRequests::Bus::BroadcastResult(editor, &AzToolsFramework::EditorRequests::GetEditor);
        if (editor)
        {
            editor->RegisterNotifyListener(this);
        }
    }

    void EditorSystemComponent::UnregisterForEditorEvents()
    {
        IEditor* editor = nullptr;
        AzToolsFramework::EditorRequests::Bus::BroadcastResult(editor, &AzToolsFramework::EditorRequests::GetEditor);
        if (editor)
        {
            editor->UnregisterNotifyListener(this);
        }
    }

    void EditorSystemComponent::OnPvdConfigurationChanged(const PhysX::Debug::PvdConfiguration& config)
    {
        auto* debug = AZ::Interface<PhysX::Debug::PhysXDebugInterface>::Get();
        if (!debug)
        {
            return;
        }

        if (config.IsAutoConnectionEditorMode())
        {
            debug->ConnectToPvd();
        }
        else
        {
            debug->DisconnectFromPvd();
        }
    }

    void EditorSystemComponent::OnStartPlayInEditorBegin()
    {
        auto* debug = AZ::Interface<PhysX::Debug::PhysXDebugInterface>::Get();
        if (!debug)
        {
            return;
        }
        
        if (debug->GetPhysXPvdConfiguration().IsAutoConnectionGameMode())
        {
            debug->ConnectToPvd();
        }
    }

    void EditorSystemComponent::OnStopPlayInEditor()
    {
        auto* debug = AZ::Interface<PhysX::Debug::PhysXDebugInterface>::Get();
        if (!debug)
        {
            return;
        }
        const PhysX::Debug::PvdConfiguration& pvdConfig = debug->GetPhysXPvdConfiguration();
        if (pvdConfig.IsAutoConnectionGameMode())
        {
            debug->DisconnectFromPvd();
        }

        if (pvdConfig.IsAutoConnectionEditorMode() && pvdConfig.m_reconnect)
        {
            debug->ConnectToPvd();
        }
    }

    void EditorSystemComponent::AutoConnectPVD()
    {
        auto* debug = AZ::Interface<PhysX::Debug::PhysXDebugInterface>::Get();
        if (!debug)
        {
            return;
        }

        if (debug->GetPhysXPvdConfiguration().IsAutoConnectionEditorMode())
        {
            debug->ConnectToPvd();
        }
    }

    void EditorSystemComponent::OnColliderProximityVisualizationChanged(const PhysX::Debug::ColliderProximityVisualization& visualizationData)
    {
        if (visualizationData.m_enabled &&
            m_cameraPositionCache.GetDistance(visualizationData.m_cameraPosition) > visualizationData.m_radius * 0.5f)
        {
            m_cameraPositionCache = visualizationData.m_cameraPosition;
        }
    }
}
