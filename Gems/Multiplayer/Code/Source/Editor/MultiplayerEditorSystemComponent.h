/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Multiplayer/IMultiplayer.h>
#include <Multiplayer/MultiplayerEditorServerBus.h>
#include <Multiplayer/Editor/MultiplayerPythonEditorEventsBus.h>
#include <IEditor.h>

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/Console/IConsole.h>
#include <AzFramework/Entity/GameEntityContextBus.h>
#include <AzFramework/Process/ProcessWatcher.h>
#include <AzFramework/Process/ProcessCommunicatorTracePrinter.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>

namespace AzNetworking
{
    class INetworkInterface;
}

namespace Multiplayer
{
    //! A component to reflect scriptable commands for the Editor
    class PythonEditorFuncs : public AZ::Component
    {
    public:
        AZ_COMPONENT(PythonEditorFuncs, "{22AEEA59-94E6-4033-B67D-7C8FBB84DF0D}")

        SANDBOX_API static void Reflect(AZ::ReflectContext* context);

        // AZ::Component ...
        void Activate() override {}
        void Deactivate() override {}
    };


    //! Multiplayer system component wraps the bridging logic between the game and transport layer.
    class MultiplayerEditorSystemComponent final
        : public AZ::Component
        , public MultiplayerEditorLayerPythonRequestBus::Handler
        , private AzFramework::GameEntityContextEventBus::Handler
        , private AzToolsFramework::EditorEvents::Bus::Handler
        , private IEditorNotifyListener
        , private MultiplayerEditorServerRequestBus::Handler
        , private AZ::TickBus::Handler
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

        //! MultiplayerEditorLayerPythonRequestBus::Handler overrides.
        //! @{
        void EnterGameMode() override;
        bool IsInGameMode() override;
        //! @}

    private:
        void LaunchEditorServer();
        
        //! EditorEvents::Handler overrides
        //! @{
        void OnEditorNotifyEvent(EEditorNotifyEvent event) override;
        //! @}
        
        //!  GameEntityContextEventBus::Handler overrides
        //! @{
        void OnGameEntitiesStarted() override;
        void OnGameEntitiesReset() override;
        //! @}

        //! MultiplayerEditorServerRequestBus::Handler
        //! @{
        void SendEditorServerLevelDataPacket(AzNetworking::IConnection* connection) override;
        //! @}

        //! AZ::TickBus::Handler
        //! @{
        void OnTick(float, AZ::ScriptTimePoint) override;
        //! @}

        IEditor* m_editor = nullptr;
        AZStd::unique_ptr<AzFramework::ProcessWatcher> m_serverProcessWatcher = nullptr;
        AZStd::unique_ptr<ProcessCommunicatorTracePrinter> m_serverProcessTracePrinter = nullptr;
        AzNetworking::ConnectionId m_editorConnId;

        ServerAcceptanceReceivedEvent::Handler m_serverAcceptanceReceivedHandler;
    };
}
