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
#include <AzCore/IO/ByteContainerStream.h>
#include <AzFramework/Entity/GameEntityContextBus.h>
#include <AzFramework/Process/ProcessWatcher.h>
#include <AzFramework/Process/ProcessCommunicatorTracePrinter.h>
#include <AzFramework/Viewport/ScreenGeometry.h>
#include <AzToolsFramework/ActionManager/ActionManagerRegistrationNotificationBus.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
#include <AzToolsFramework/Prefab/Spawnable/PrefabToInMemorySpawnableNotificationBus.h>

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

        static void Reflect(AZ::ReflectContext* context);

        // AZ::Component ...
        void Activate() override {}
        void Deactivate() override {}
    };

    //! Multiplayer system component wraps the bridging logic between the game and transport layer.
    class MultiplayerEditorSystemComponent final
        : public AZ::Component
        , public MultiplayerEditorLayerPythonRequestBus::Handler
        , private AzToolsFramework::EditorEvents::Bus::Handler
        , private IEditorNotifyListener
        , private MultiplayerEditorServerRequestBus::Handler
        , private AZ::TickBus::Handler
        , private AzToolsFramework::Prefab::PrefabToInMemorySpawnableNotificationBus::Handler
        , private AzToolsFramework::EditorEntityContextNotificationBus::Handler
        , private AzToolsFramework::ActionManagerRegistrationNotificationBus::Handler
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
        bool LaunchEditorServer();
        bool FindServerLauncher(AZ::IO::FixedMaxPath& serverPath);
        void Connect();

        //! AzToolsFramework::EditorEntityContextNotificationBus::Handler overrides
        //! @{
        void OnStartPlayInEditorBegin() override;
        void OnStartPlayInEditor() override;
        void OnStopPlayInEditorBegin() override;
        //! @}

        // AzToolsFramework::ActionManagerRegistrationNotificationBus overrides ...
        void OnActionRegistrationHook() override;
        void OnMenuBindingHook() override;

        //! AzToolsFramework::Prefab::PrefabToInMemorySpawnableNotificationBus::Handler overrides
        //! @{
        void OnPreparingInMemorySpawnableFromPrefab(const AzFramework::Spawnable& spawnable, const AZStd::string& assetHint) override;
        //! @}

        //! EditorEvents::Handler overrides
        //! @{
        void OnEditorNotifyEvent(EEditorNotifyEvent event) override;
        //! @}

        //! MultiplayerEditorServerRequestBus::Handler
        //! @{
        void SendEditorServerLevelDataPacket(AzNetworking::IConnection* connection) override;
        //! @}

        //! AZ::TickBus::Handler
        //! @{
        void OnTick(float, AZ::ScriptTimePoint) override;
        //! @}

        //! Context menu handler
        void ContextMenu_NewMultiplayerEntity(AZ::EntityId parentEntityId, const AZ::Vector3& worldPosition);

        void ResetLevelSendData();
        void SendLevelDataToServer();

        IEditor* m_editor = nullptr;
        AZStd::unique_ptr<AzFramework::ProcessWatcher> m_serverProcessWatcher = nullptr;
        AZStd::unique_ptr<ProcessCommunicatorTracePrinter> m_serverProcessTracePrinter = nullptr;
        AzNetworking::ConnectionId m_editorConnId;

        ServerAcceptanceReceivedEvent::Handler m_serverAcceptanceReceivedHandler;
        AZ::ScheduledEvent m_connectionEvent = AZ::ScheduledEvent([this]{this->Connect();}, AZ::Name("MultiplayerEditorConnect"));
        uint16_t m_connectionAttempts = 0;

        struct PreAliasedSpawnableData
        {
            AZStd::unique_ptr<AzFramework::Spawnable> spawnable;
            AZStd::string assetHint;
            AZ::Data::AssetId assetId;
        };

        AZStd::vector<PreAliasedSpawnableData> m_preAliasedSpawnablesForServer;

        // Structure that encapsulates the data we need for sending the level data to the server when entering game mode.
        struct LevelSendData
        {
            AZStd::vector<uint8_t> m_sendBuffer;
            AZStd::unique_ptr<AZ::IO::ByteContainerStream<AZStd::vector<uint8_t>>> m_byteStream;
            AzNetworking::IConnection* m_sendConnection = nullptr;
        };

        LevelSendData m_levelSendData;
    };
}
