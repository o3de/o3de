/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Multiplayer/IMultiplayer.h>

#include <IEditor.h>

#include <Editor/MultiplayerEditorConnection.h>

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/Console/IConsole.h>
#include <AzCore/Console/ILogger.h>
#include <AzFramework/Entity/GameEntityContextBus.h>
#include <AzFramework/Process/ProcessWatcher.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>

namespace AzNetworking
{
    class INetworkInterface;
}

namespace Multiplayer
{
    //! Multiplayer system component wraps the bridging logic between the game and transport layer.
    class MultiplayerEditorSystemComponent final
        : public AZ::Component
        , private AzFramework::GameEntityContextEventBus::Handler
        , private AzToolsFramework::EditorEvents::Bus::Handler
        , private IEditorNotifyListener
    {
    public:
        AZ_COMPONENT(MultiplayerEditorSystemComponent, "{9F335CC0-5574-4AD3-A2D8-2FAEF356946C}");

        static void Reflect(AZ::ReflectContext* context);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);

        MultiplayerEditorSystemComponent();
        ~MultiplayerEditorSystemComponent() override = default;

        //! Called once the editor receives the server's accept packet
        void OnServerAcceptanceReceived();

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
        
        //!  GameEntityContextEventBus::Handler overrides
        //! @{
        void OnGameEntitiesStarted() override;
        void OnGameEntitiesReset() override;
        //! @}

        IEditor* m_editor = nullptr;
        AzFramework::ProcessWatcher* m_serverProcess = nullptr;
        AzNetworking::ConnectionId m_editorConnId;

        ServerAcceptanceReceivedEvent::Handler m_serverAcceptanceReceivedHandler;
    };
}
