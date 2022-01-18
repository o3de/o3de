/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzFramework/Physics/Common/PhysicsEvents.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <IEditor.h>

#include <PhysX/Debug/PhysXDebugInterface.h>

namespace PhysXDebug
{
    class EditorSystemComponent
        : public AzToolsFramework::Components::EditorComponentBase
        , protected AzToolsFramework::EditorEntityContextNotificationBus::Handler
        , public CrySystemEventBus::Handler
        , private AzToolsFramework::EditorEvents::Bus::Handler
        , private IEditorNotifyListener
    {
    public:
        AZ_COMPONENT(EditorSystemComponent, "{E6F88D74-5758-453E-8FE0-2FB5E5E53890}");

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("PhysXDebugEditorService", 0xf8611967));
        }

        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC("PhysXService", 0x75beae2d));
        }

    protected:
        // AZ::Component
        void Activate() override;
        void Deactivate() override;

        // CrySystemEvents
        void OnCrySystemShutdown(ISystem&) override;

        // AzToolsFramework::EditorEvents
        void NotifyRegisterViews() override;
    private:
        // EditorEntityContextNotificationBus interface implementation
        void OnStartPlayInEditorBegin() override;
        void OnStopPlayInEditor() override;

        // IEditorNotifyListener interface implementation
        void OnEditorNotifyEvent(EEditorNotifyEvent event) override;

        /// Initially connect to the PhysX Visualization debugger based on the current PhysX configuration.
        void AutoConnectPVD();

        /// Register for Cry Editor events.
        void RegisterForEditorEvents();

        /// Unregister for Cry Editor events.
        void UnregisterForEditorEvents();

        void OnColliderProximityVisualizationChanged(const PhysX::Debug::ColliderProximityVisualization& visualizationData);
        PhysX::Debug::ColliderProximityVisualizationChangedEvent::Handler m_colliderProximityVisualizationChangedEventHandler;
        AZ::Vector3 m_cameraPositionCache = AZ::Vector3::CreateZero();

        void OnPvdConfigurationChanged(const PhysX::Debug::PvdConfiguration& config);
        PhysX::Debug::PvdConfigurationChangedEvent::Handler m_pvdConfigurationChangeHandler;
    };
}
