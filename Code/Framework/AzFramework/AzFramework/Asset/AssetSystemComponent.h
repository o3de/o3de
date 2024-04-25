/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/string/string_view.h>
#include <AzFramework/Asset/AssetSystemBus.h>
#include <AzFramework/Network/SocketConnection.h>
#include <AzFramework/Asset/AssetCatalogBus.h>

namespace AzFramework
{
    namespace AssetSystem
    {
        constexpr char BranchToken[] = "assetProcessor_branch_token";
        constexpr char Assets[] = "assets";
        constexpr char AssetProcessorRemoteIp[] = "remote_ip";
        constexpr char AssetProcessorRemotePort[] = "remote_port";
        constexpr char WaitForConnect[] = "wait_for_connect";

        /**
        * A game level component for interacting with the asset processor
        *
        * Currently used to request synchronous asset compilation, provide notifications
        * when assets are updated, and to query asset status
        */
        class AssetSystemComponent
            : public AZ::Component
            , private AssetSystemRequestBus::Handler
            , private AZ::SystemTickBus::Handler
        {
        public:
            AZ_COMPONENT(AssetSystemComponent, "{42C58BBF-0C15-4DF9-9351-4639B36F122A}")

            AssetSystemComponent() = default;
            virtual ~AssetSystemComponent() = default;

            //////////////////////////////////////////////////////////////////////////
            // AZ::Component overrides
            void Init() override;
            void Activate() override;
            void Deactivate() override;
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            // SystemTickBus overrides
            void OnSystemTick() override;
            //////////////////////////////////////////////////////////////////////////

            static void Reflect(AZ::ReflectContext* context);
            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);

        private:
            AssetSystemComponent(const AssetSystemComponent&) = delete;
            void EnableSocketConnection();
            void DisableSocketConnection();
        public:
            //////////////////////////////////////////////////////////////////////////
            // AssetSystemRequestBus::Handler overrides

            //! Uses the ConnectionsSettings ConnectionDirection field
            //! to determine whether to connect to an AssetProcessor instance or to listen for
            //! a connection
            bool EstablishAssetProcessorConnection(const ConnectionSettings& connectionSettings) override;

            bool WaitUntilAssetProcessorConnected(AZStd::chrono::duration<float> timeout) override;
            bool WaitUntilAssetProcessorReady(AZStd::chrono::duration<float> timeout) override;

            bool AssetProcessorIsReady() override;
            bool ConnectedWithAssetProcessor() override;
            bool NegotiationWithAssetProcessorFailed() override;

            void StartDisconnectingAssetProcessor() override;
            bool DisconnectedWithAssetProcessor() override;
            bool WaitUntilAssetProcessorDisconnected(AZStd::chrono::duration<float> timeout) override;

        private:

            //! Sets the asset processor IP to use when connecting
            //! \return Whether the ip was set, can fail if already connected
            bool SetAssetProcessorIP(AZStd::string_view ip);
            //! Sets the asset processor port to use when connecting
            //! \return Whether the port was set, can fail if already connected
            bool SetAssetProcessorPort(AZ::u16 port) ;
            //! Sets the branchtoken that will be used for negotiating with the AssetProcessor
            void SetAssetProcessorBranchToken(AZStd::string_view branchtoken);
            //! Sets the (game) project name that will be used for negotiating with the AssetProcessor
            void SetAssetProcessorProjectName(AZStd::string_view projectName);
            //! Sets the platform that will be used for negotiating with the AssetProcessor
            void SetAssetProcessorPlatform(AZStd::string_view platform);
            //! Sets the identifier that will be used for negotiating with the AssetProcessor
            void SetAssetProcessorIdentifier(AZStd::string_view identifier);

            //! Configure the underlying socket connection
            //! \return Whether the ip was set, can fail if already connected
            bool ConfigureSocketConnection();

            //! Start the connection thread that will try to initiate a connection to the Asset Processor.
            //! Once successfully called, the connection thread will starts trying to connect to the ap and will keep trying to initiate a connection, until StartDiconnectingAssetProcessor is called
            //! return of true does NOT mean it connected, only that it successfully started the connection thread. You have to keep checking and/or wait is desired
            //! \return True if connect was called, NOT that it connected, so true means the connection thread was started. False means connect was not called and therefore
            //! the connection thread was not started. It can fail if already connected or the connection was not configured prior to this call
            bool StartConnectToAssetProcessor();

            //! Convenience function that calls StartConnectToAssetProcessor and then waits for timeout seconds for a connection, if not connected before timeout, StartDiconnectingAssetProcessor is
            //! called to stop the connection thread. A timeout of 0 means wait forever until connected, this is not recommended as you will have more control by doing that yourself
            //! in the calling code.
            //! returns true if successfully connected, false if not
            bool ConnectToAssetProcessor(const ConnectionSettings& connectionSettings);

            //! Start the connection thread that will try to listen for an Asset Processor to initiate a connection to us.
            //! Once successfully called, the connection thread starts listening for an asset processor to initiate a connection to us and will keep trying to initiate a connection, until Disconnect is called
            //! \return True if listen was called, NOT that it connected, so true means the connection thread was started. False means listen was not called and therefore
            //! the connection thread was not started. It can fail if already connected or the connection was not configured prior to this call
            bool StartConnectFromAssetProcessor();

            //! Convenience function that calls StartConnectToAssetProcessor and then waits for timeout seconds for a connection, if not connected before timeout, StartDiconnectingAssetProcessor is
            //! called to stop the connection thread. A timeout of 0 means wait forever until connected, this is not recommended as you will have more control by doing that yourself
            //! in the calling code.
            //! returns true if successfully connected, false if not
            bool ConnectFromAssetProcessor(const ConnectionSettings& connectionSettings);

            AssetStatus CompileAssetSync(const AZStd::string& assetPath) override;
            AssetStatus CompileAssetSync_FlushIO(const AZStd::string& assetPath) override;

            AssetStatus CompileAssetSyncById(const AZ::Data::AssetId& assetId) override;
            AssetStatus CompileAssetSyncById_FlushIO(const AZ::Data::AssetId& assetId) override;

            AssetStatus GetAssetStatusSearchType(const AZStd::string& assetPath, int searchType) override;
            AssetStatus GetAssetStatusSearchType_FlushIO(const AZStd::string& searchTerm, int searchType) override;
            AssetStatus GetAssetStatusById(const AZ::Data::AssetId& assetId) override;
            AssetStatus GetAssetStatusById_FlushIO(const AZ::Data::AssetId& assetId) override;

            AssetStatus GetAssetStatus(const AZStd::string& assetPath) override;
            AssetStatus GetAssetStatus_FlushIO(const AZStd::string& assetPath) override;

            bool EscalateAssetByUuid(const AZ::Uuid& assetUuid) override;
            bool EscalateAssetBySearchTerm(AZStd::string_view searchTerm) override;

            void ShowAssetProcessor() override;
            void UpdateSourceControlStatus(bool newStatus) override;
            void ShowInAssetProcessor(const AZStd::string& assetPath) override;

            void GetUnresolvedProductReferences(AZ::Data::AssetId assetId, AZ::u32& unresolvedAssetIdReferences, AZ::u32& unresolvedPathReferences) override;

            float GetAssetProcessorPingTimeMilliseconds() override;

            bool SaveCatalog() override;
            //////////////////////////////////////////////////////////////////////////

            AssetStatus SendAssetStatusRequest(const RequestAssetStatus& request);

            AZStd::unique_ptr<SocketConnection> m_socketConn = nullptr;
            SocketConnection::TMessageCallbackHandle m_cbHandle = 0;
            SocketConnection::TMessageCallbackHandle m_bulkMessageHandle = 0;
            AZStd::string m_assetProcessorBranchToken;
            AZStd::string m_assetProcessorProjectName;
            AZStd::string m_assetProcessorPlatform;
            AZStd::string m_assetProcessorIdentifier;
            AZStd::string m_assetProcessorIP;
            AZ::u16 m_assetProcessorPort = 45643;
            bool m_configured = false;
        };
    } // namespace AssetSystem
} // namespace AzFramework

