/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Asset/AssetSystemComponent.h>

#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/Debug/Profiler.h>
#include <AzCore/std/string/conversions.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/IStreamer.h>
#include <AzCore/IO/Streamer/FileRequest.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/std/chrono/chrono.h>
#include <AzCore/std/string/conversions.h>
#include <AzCore/StringFunc/StringFunc.h>

#include <AzFramework/API/ApplicationAPI.h>
#include <AzFramework/Asset/AssetCatalogBus.h>
#include <AzFramework/Asset/AssetProcessorMessages.h>
#include <AzFramework/Asset/AssetSeedList.h>
#include <AzFramework/Asset/NetworkAssetNotification_private.h>
#include <AzFramework/Asset/Benchmark/BenchmarkCommands.h>
#include <AzFramework/Network/AssetProcessorConnection.h>

DECLARE_EBUS_INSTANTIATION(AzFramework::AssetSystem::AssetSystemRequests);

AZ_DECLARE_BUDGET(AzFramework);

namespace AzFramework
{
    namespace AssetBenchmark
    {
        // These are specifically registered here instead of in BenchmarkCommands.cpp to avoid dead code stripping.
        // If BenchmarkCommands.cpp only contained the functions and the calls to AZ_CONSOLEFREEFUNC, there would be
        // no external references into that compilation unit, which signals the linker to remove the entire file.
        AZ_CONSOLEFREEFUNC(BenchmarkLoadAssetList, AZ::ConsoleFunctorFlags::Null, "Time the loading of all the listed asset names.  "
            "This will load any assets previously added via 'BenchmarkAddAssetToList' as well as any directly listed with this command.");

        AZ_CONSOLEFREEFUNC(BenchmarkClearAssetList, AZ::ConsoleFunctorFlags::Null,
            "Clear the list of assets to load with 'BenchmarkLoadAssetList'");
        AZ_CONSOLEFREEFUNC(BenchmarkAddAssetsToList, AZ::ConsoleFunctorFlags::Null,
            "Add asset(s) to the list of assets to load with 'BenchmarkLoadAssetList'");

        AZ_CONSOLEFREEFUNC(BenchmarkLoadAllAssets, AZ::ConsoleFunctorFlags::Null, "Time the loading of all assets in the catalog");
        AZ_CONSOLEFREEFUNC(BenchmarkLoadAllAssetsSynchronous, AZ::ConsoleFunctorFlags::Null,
            "Time the loading of all assets in the catalog synchronously");
    }

    namespace AssetSystem
    {
        void OnAssetSystemMessage(unsigned int /*typeId*/, const void* buffer, unsigned int bufferSize, AZ::SerializeContext* context)
        {
            AssetNotificationMessage message;

            // note that we forbid asset loading and we set STRICT mode.  These messages are all the kind of message that is supposed to be transmitted between the
            // same version of software, and are created at runtime, not loaded from disk, so they should not contain errors - if they do, it requires investigation.
            if (!AZ::Utils::LoadObjectFromBufferInPlace(buffer, bufferSize, message, context, AZ::ObjectStream::FilterDescriptor(&AZ::Data::AssetFilterNoAssetLoading, AZ::ObjectStream::FILTERFLAG_STRICT)))
            {
                AZ_WarningOnce("AssetSystem", false, "AssetNotificationMessage received but unable to deserialize it.  Is AssetProcessor.exe up to date?");
                return;
            }

            if (message.m_data.length() > AZ_MAX_PATH_LEN)
            {
                auto maxPath = message.m_data.substr(0, AZ_MAX_PATH_LEN - 1);
                AZ_Warning("AssetSystem", false, "HotUpdate: filename too long(%zd) : %s...", bufferSize, maxPath.c_str());
                return;
            }

            switch (message.m_type)
            {
            case AssetNotificationMessage::AssetChanged:
            {
                // Used only to communicate to AssetCatalogs - no other system should rely on this
                // Instead listen to AssetCatalogEventBus::OnAssetChanged
                // This is a DIRECT call so that the catalog can update itself immediately so that it maintains as accurate a view of the current state of assets as possible.
                // Attempting to queue this on the main thread has led to issues previously where systems start receiving
                // asset change/remove notifications before all of the network catalog updates have been applied and start querying the state of other assets which haven't been updated yet.
                AzFramework::AssetSystem::NetworkAssetUpdateInterface* notificationInterface = AZ::Interface<AzFramework::AssetSystem::NetworkAssetUpdateInterface>::Get();
                if (notificationInterface)
                {
                    notificationInterface->AssetChanged({ message });
                }
            }
            break;
            case AssetNotificationMessage::AssetRemoved:
            {
                // Used only to communicate to AssetCatalogs - no other system should rely on this
                // Instead listen to AssetCatalogEventBus::OnAssetRemoved
                // This is a DIRECT call so that the catalog can update itself immediately so that it maintains as accurate a view of the current state of assets as possible.
                // Attempting to queue this on the main thread has led to issues previously where systems start receiving
                // asset change/remove notifications before all of the network catalog updates have been applied and start querying the state of other assets which haven't been updated yet.
                AzFramework::AssetSystem::NetworkAssetUpdateInterface* notificationInterface = AZ::Interface<AzFramework::AssetSystem::NetworkAssetUpdateInterface>::Get();
                if (notificationInterface)
                {
                    notificationInterface->AssetRemoved({ message });
                }
            }
            break;
            case AssetNotificationMessage::JobFileClaimed:
            {
                auto streamer = AZ::Interface<AZ::IO::IStreamer>::Get();
                if (streamer)
                {
                    // The Asset Processor is about to write a product file that could have been loaded through AZ::IO::Streamer so
                    // flush it from any caches so stale data isn't used on reload and to make sure any file handles are released.
                    streamer->QueueRequest(streamer->FlushCache(message.m_data));
                }
                AssetSystemInfoBus::Broadcast(&AssetSystemInfoBus::Events::AssetFileClaimed, message.m_data);
            }
            break;
            case AssetNotificationMessage::JobFileReleased:
            {
                AssetSystemInfoBus::Broadcast(&AssetSystemInfoBus::Events::AssetFileReleased, message.m_data);
            }
            break;
            case AssetNotificationMessage::JobStarted:
            {
                AssetSystemInfoBus::Broadcast(&AssetSystemInfoBus::Events::AssetCompilationStarted, message.m_data);
            }
            break;
            case AssetNotificationMessage::JobCompleted:
            {
                AssetSystemInfoBus::Broadcast(&AssetSystemInfoBus::Events::AssetCompilationSuccess, message.m_data);
            }
            break;
            case AssetNotificationMessage::JobFailed:
            {
                AssetSystemInfoBus::Broadcast(&AssetSystemInfoBus::Events::AssetCompilationFailed, message.m_data);
            }
            break;
            case AssetNotificationMessage::JobCount:
            {
                int numberOfAssets = atoi(message.m_data.c_str());
                AssetSystemInfoBus::Broadcast(&AssetSystemInfoBus::Events::CountOfAssetsInQueue, numberOfAssets);
            }
            break;
            default:
                AZ_WarningOnce("AssetSystem", false, "Unknown AssetNotificationMessage type received from network.  Is AssetProcessor.exe up to date?");
                break;
            }
        }

        void AssetSystemComponent::Init()
        {
            m_socketConn.reset(new AssetProcessorConnection());
        }

        void AssetSystemComponent::Activate()
        {
            AZ::SerializeContext* context = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(context, &AZ::ComponentApplicationRequests::GetSerializeContext);

            EnableSocketConnection();

            m_cbHandle = m_socketConn->AddMessageHandler(AssetNotificationMessage::MessageType,
                [context](unsigned int typeId, unsigned int /*serial*/, const void* data, unsigned int dataLength)
            {
                if (dataLength)
                {
                    OnAssetSystemMessage(typeId, data, dataLength, context);
                }
            });

            m_bulkMessageHandle = m_socketConn->AddMessageHandler(
                BulkAssetNotificationMessage::MessageType,
                [context](unsigned int /*typeId*/, unsigned int /*serial*/, const void* data, unsigned int dataLength)
                {
                    if (dataLength)
                    {
                        BulkAssetNotificationMessage bulkMessage;

                        // note that we forbid asset loading and we set STRICT mode.  These messages are all the kind of message that is
                        // supposed to be transmitted between the same version of software, and are created at runtime, not loaded from
                        // disk, so they should not contain errors - if they do, it requires investigation.
                        if (!AZ::Utils::LoadObjectFromBufferInPlace(
                                data,
                                dataLength,
                                bulkMessage,
                                context,
                                AZ::ObjectStream::FilterDescriptor(
                                    &AZ::Data::AssetFilterNoAssetLoading, AZ::ObjectStream::FILTERFLAG_STRICT)))
                        {
                            AZ_WarningOnce(
                                "AssetSystem",
                                false,
                                "BulkAssetNotificationMessage received but unable to deserialize it.  Is AssetProcessor.exe up to date?");
                            return;
                        }

                        AzFramework::AssetSystem::NetworkAssetUpdateInterface* notificationInterface =
                            AZ::Interface<AzFramework::AssetSystem::NetworkAssetUpdateInterface>::Get();

                        if (!notificationInterface)
                        {
                            return;
                        }

                        switch(bulkMessage.m_type)
                        {
                        case AssetNotificationMessage::AssetChanged:
                            notificationInterface->AssetChanged(bulkMessage.m_messages, true);
                            break;
                        case AssetNotificationMessage::AssetRemoved:
                            notificationInterface->AssetRemoved(bulkMessage.m_messages);
                            break;
                        default:
                            AZ_Warning(
                                "AssetSystem",
                                false,
                                "BulkAssetNotificationMessage received with invalid/unsupported type %d",
                                int(bulkMessage.m_type));
                        }
                    }
                });

            AssetSystemRequestBus::Handler::BusConnect();
            AZ::SystemTickBus::Handler::BusConnect();

            AssetSystemStatusBus::Broadcast(&AssetSystemStatusBus::Events::AssetSystemAvailable);
        }

        void AssetSystemComponent::Deactivate()
        {
            AssetSystemStatusBus::Broadcast(&AssetSystemStatusBus::Events::AssetSystemUnavailable);

            AZ::SystemTickBus::Handler::BusDisconnect();
            AssetSystemRequestBus::Handler::BusDisconnect();
            m_socketConn->RemoveMessageHandler(AssetNotificationMessage::MessageType, m_cbHandle);
            m_socketConn->RemoveMessageHandler(BulkAssetNotificationMessage::MessageType, m_bulkMessageHandle);
            m_socketConn->Disconnect(true);

            DisableSocketConnection();
        }

        void AssetSystemComponent::Reflect(AZ::ReflectContext* context)
        {
            NegotiationMessage::Reflect(context);
            BaseAssetProcessorMessage::Reflect(context);
            RequestAssetStatus::Reflect(context);
            RequestEscalateAsset::Reflect(context);
            ResponseAssetProcessorStatus::Reflect(context);
            RequestAssetProcessorStatus::Reflect(context);
            ResponseAssetStatus::Reflect(context);

            RequestPing::Reflect(context);
            ResponsePing::Reflect(context);

            // Requests
            GetUnresolvedDependencyCountsRequest::Reflect(context);
            GetRelativeProductPathFromFullSourceOrProductPathRequest::Reflect(context);
            GenerateRelativeSourcePathRequest::Reflect(context);
            GetFullSourcePathFromRelativeProductPathRequest::Reflect(context);
            SourceAssetInfoRequest::Reflect(context);
            AssetInfoRequest::Reflect(context);
            AssetDependencyInfoRequest::Reflect(context);
            RegisterSourceAssetRequest::Reflect(context);
            UnregisterSourceAssetRequest::Reflect(context);
            ShowAssetProcessorRequest::Reflect(context);
            UpdateSourceControlStatusRequest::Reflect(context);
            ShowAssetInAssetProcessorRequest::Reflect(context);
            FileOpenRequest::Reflect(context);
            FileCloseRequest::Reflect(context);
            FileReadRequest::Reflect(context);
            FileWriteRequest::Reflect(context);
            FileTellRequest::Reflect(context);
            FileSeekRequest::Reflect(context);
            FileIsReadOnlyRequest::Reflect(context);
            PathIsDirectoryRequest::Reflect(context);
            FileSizeRequest::Reflect(context);
            FileModTimeRequest::Reflect(context);
            FileExistsRequest::Reflect(context);
            FileFlushRequest::Reflect(context);
            PathCreateRequest::Reflect(context);
            PathDestroyRequest::Reflect(context);
            FileRemoveRequest::Reflect(context);
            FileCopyRequest::Reflect(context);
            FileRenameRequest::Reflect(context);
            FindFilesRequest::Reflect(context);

            FileTreeRequest::Reflect(context);

            AssetChangeReportRequest::Reflect(context);

            // Responses
            GetUnresolvedDependencyCountsResponse::Reflect(context);
            GetRelativeProductPathFromFullSourceOrProductPathResponse::Reflect(context);
            GenerateRelativeSourcePathResponse::Reflect(context);
            GetFullSourcePathFromRelativeProductPathResponse::Reflect(context);
            SourceAssetInfoResponse::Reflect(context);
            AssetInfoResponse::Reflect(context);
            AssetDependencyInfoResponse::Reflect(context);
            FileOpenResponse::Reflect(context);
            FileReadResponse::Reflect(context);
            FileWriteResponse::Reflect(context);
            FileTellResponse::Reflect(context);
            FileSeekResponse::Reflect(context);
            FileIsReadOnlyResponse::Reflect(context);
            PathIsDirectoryResponse::Reflect(context);
            FileSizeResponse::Reflect(context);
            FileModTimeResponse::Reflect(context);
            FileExistsResponse::Reflect(context);
            FileFlushResponse::Reflect(context);
            PathCreateResponse::Reflect(context);
            PathDestroyResponse::Reflect(context);
            FileRemoveResponse::Reflect(context);
            FileCopyResponse::Reflect(context);
            FileRenameResponse::Reflect(context);
            FindFilesResponse::Reflect(context);

            FileTreeResponse::Reflect(context);

            AssetChangeReportResponse::Reflect(context);

            SaveAssetCatalogRequest::Reflect(context);
            SaveAssetCatalogResponse::Reflect(context);

            AssetNotificationMessage::Reflect(context);
            BulkAssetNotificationMessage::Reflect(context);
            AssetSeedListReflector::Reflect(context);
            SeedInfo::Reflect(context);
            AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
            if (serialize)
            {
                serialize->Class<AssetSystemComponent, AZ::Component>()
                    ;
            }
        }

        void AssetSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC_CE("AssetProcessorConnection"));
        }

        void AssetSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC_CE("AssetProcessorConnection"));
        }

        void AssetSystemComponent::EnableSocketConnection()
        {
            AZ_Assert(!SocketConnection::GetInstance(), "You can only have one AZ::SocketConnection");
            if (!SocketConnection::GetInstance())
            {
                SocketConnection::SetInstance(m_socketConn.get());
            }
        }

        void AssetSystemComponent::DisableSocketConnection()
        {
            SocketConnection* socketConnection = SocketConnection::GetInstance();
            if (socketConnection && socketConnection == m_socketConn.get())
            {
                SocketConnection::SetInstance(nullptr);
            }
        }

        //////////////////////////////////////////////////////////////////////////
        // SystemTickBus overrides
        void AssetSystemComponent::OnSystemTick()
        {
            AZ_PROFILE_FUNCTION(AzFramework);
            LegacyAssetEventBus::ExecuteQueuedEvents();
        }

        bool AssetSystemComponent::SetAssetProcessorIP(AZStd::string_view ip)
        {
            if (ConnectedWithAssetProcessor())
            {
                AZ_Warning("AssetSystem", false, "Cannot change ip while already connected");
                return false;
            }

            m_assetProcessorIP = ip;
            return true;
        }

        bool AssetSystemComponent::SetAssetProcessorPort(AZ::u16 port)
        {
            if (ConnectedWithAssetProcessor())
            {
                AZ_Warning("AssetSystem", false, "Cannot change port while already connected");
                return false;
            }

            m_assetProcessorPort = port;
            return true;
        }

        void AssetSystemComponent::SetAssetProcessorBranchToken(AZStd::string_view branchToken)
        {
            if (m_assetProcessorBranchToken != branchToken)
            {
                m_configured = false;
                m_assetProcessorBranchToken = branchToken;
            }
        }

        void AssetSystemComponent::SetAssetProcessorProjectName(AZStd::string_view projectName)
        {
            if (m_assetProcessorProjectName != projectName)
            {
                m_configured = false;
                m_assetProcessorProjectName = projectName;
            }
        }

        void AssetSystemComponent::SetAssetProcessorPlatform(AZStd::string_view platform)
        {
            if (m_assetProcessorPlatform != platform)
            {
                m_configured = false;
                m_assetProcessorPlatform = platform;
            }
        }

        void AssetSystemComponent::SetAssetProcessorIdentifier(AZStd::string_view identifier)
        {
            if (m_assetProcessorIdentifier != identifier)
            {
                m_configured = false;
                m_assetProcessorIdentifier = identifier;
            }
        }

        bool AssetSystemComponent::ConfigureSocketConnection()
        {
            AZ_Assert(m_socketConn.get(), "SocketConnection doesn't exist!  Ensure AssetSystemComponent::Init was called");

            if (ConnectedWithAssetProcessor()) // Don't allow changing the IP while connected
            {
                AZ_Warning("AssetSystem", false, "Cannot configure while connected");
                return false;
            }

            auto apConnection = azrtti_cast<AssetProcessorConnection*>(m_socketConn.get());
            apConnection->Configure(m_assetProcessorBranchToken.c_str(),
                m_assetProcessorPlatform.c_str(),
                m_assetProcessorIdentifier.c_str(),
                m_assetProcessorProjectName.c_str());
            m_configured = true;
            return true;
        }


        bool AssetSystemComponent::StartConnectToAssetProcessor()
        {
            AZ_Assert(m_socketConn.get(), "SocketConnection doesn't exist!  Ensure AssetSystemComponent::Init was called");

            if (ConnectedWithAssetProcessor())
            {
                AZ_Warning("AssetSystem", false, "Cannot connect while already connected.");
                return false;
            }

            if (!m_configured)
            {
                AZ_Warning("AssetSystem", false, "SocketConnection was not configured before calling StartConnectToAssetProcessor!!! Ensure AssetSystemComponent::ConfigureSocketConnection was called after changing any setting.");
                return false;
            }

            //connect is async
            AZ_TracePrintf("Asset System Connection", "Asset Processor Connection IP: %s, port: %hu, branch token %s\n", m_assetProcessorIP.c_str(), m_assetProcessorPort, m_assetProcessorBranchToken.c_str());
            auto apConnection = azrtti_cast<AssetProcessorConnection*>(m_socketConn.get());
            apConnection->Connect(m_assetProcessorIP.c_str(), m_assetProcessorPort);
            return true;
        }

        bool AssetSystemComponent::StartConnectFromAssetProcessor()
        {
            AZ_Assert(m_socketConn.get(), "SocketConnection doesn't exist!  Ensure AssetSystemComponent::Init was called");

            if (ConnectedWithAssetProcessor()) // Don't allow changing the IP while connected
            {
                AZ_Warning("AssetSystem", false, "Cannot connect while connected.");
                return false;
            }

            if (!m_configured)
            {
                AZ_Warning("AssetSystem", false, "SocketConnection was not configured before calling StartConnectFromAssetProcessor!!! Ensure AssetSystemComponent::ConfigureSocketConnection was called after changing any setting.");
                return false;
            }

            //listen is async
            //instances always listen on port 22229, currently its not configurable
            auto apConnection = azrtti_cast<AssetProcessorConnection*>(m_socketConn.get());
            apConnection->Listen(22229);
            return true;
        }

        bool AssetSystemComponent::ConnectToAssetProcessor(const ConnectionSettings& connectionSettings)
        {
            if (connectionSettings.m_loggingCallback)
            {
                connectionSettings.m_loggingCallback("Connecting to Asset Processor...\n");
            }
            if (!SetAssetProcessorIP(connectionSettings.m_assetProcessorIp))
            {
                AZ_Warning("AssetSystem", false, "SetAssetProcessorIP() has failed!!!.");
                return false;
            }
            if (!SetAssetProcessorPort(connectionSettings.m_assetProcessorPort))
            {
                AZ_Warning("AssetSystem", false, "SetAssetProcessorPort() has failed!!!.");
                return false;
            }
            SetAssetProcessorBranchToken(connectionSettings.m_branchToken);
            SetAssetProcessorProjectName(connectionSettings.m_projectName);
            SetAssetProcessorPlatform(connectionSettings.m_assetPlatform);
            SetAssetProcessorIdentifier(connectionSettings.m_connectionIdentifier);
            if (!ConfigureSocketConnection())
            {
                AZ_Warning("AssetSystem", false, "ConfigureSocketConnection() has failed!!!.");
                return false;
            }

            if (!StartConnectToAssetProcessor())
            {
                AZ_Warning("AssetSystem", false, "StartConnectToAssetProcessor() has failed!!!");
                return false;
            }

            return WaitUntilAssetProcessorConnected(connectionSettings.m_connectTimeout);
        }

        bool AssetSystemComponent::ConnectFromAssetProcessor(const ConnectionSettings& connectionSettings)
        {
            if (connectionSettings.m_loggingCallback)
            {
                connectionSettings.m_loggingCallback("Listening for Asset Processor connection...\n");
            }
            SetAssetProcessorBranchToken(connectionSettings.m_branchToken);
            SetAssetProcessorProjectName(connectionSettings.m_projectName);
            SetAssetProcessorPlatform(connectionSettings.m_assetPlatform);
            SetAssetProcessorIdentifier(connectionSettings.m_connectionIdentifier);
            if(!ConfigureSocketConnection())
            {
                AZ_Warning("AssetSystem", false, "ConfigureSocketConnection() has failed!!!");
                return false;
            }
            if (!StartConnectFromAssetProcessor())
            {
                AZ_Warning("AssetSystem", false, "StartConnectFromAssetProcessor() has failed!!!");
                return false;
            }

            return WaitUntilAssetProcessorConnected(connectionSettings.m_connectTimeout);
        }

        //////////////////////////////////////////////////////////////////////////
        // AssetSystemRequestBus::Handler overrides

        bool AssetSystemComponent::EstablishAssetProcessorConnection(const ConnectionSettings& connectionSettings)
        {
            bool connectionEstablished{};
            if (connectionSettings.m_connectionDirection == ConnectionSettings::ConnectionDirection::ConnectToAssetProcessor)
            {
                connectionEstablished = ConnectToAssetProcessor(connectionSettings);
            }
            else
            {
                connectionEstablished = ConnectFromAssetProcessor(connectionSettings);
            }

            if (!connectionEstablished)
            {
                bool failedNegotiation = NegotiationWithAssetProcessorFailed();
                if (failedNegotiation)
                {
                    AZ_Error(connectionSettings.m_connectionIdentifier.c_str(), false, "Negotiation with asset processor failed");
                    return false;
                }

#if AZ_TRAIT_OS_IS_HOST_OS_PLATFORM
                if (connectionSettings.m_launchAssetProcessorOnFailedConnection)
                {
                    if (!LaunchAssetProcessor())
                    {
                        if (connectionSettings.m_waitForConnect)
                        {
                            AZ_Error(connectionSettings.m_connectionIdentifier.c_str(), false, "Launch asset processor failed");
                            return false;
                        }
                        else
                        {
                            AZ_Warning(connectionSettings.m_connectionIdentifier.c_str(), false, "Launch asset processor failed");
                        }
                    }
                    else
                    {
                        auto startConnectFromLaunchTime = AZStd::chrono::steady_clock::now();
                        connectionEstablished = WaitUntilAssetProcessorConnected(connectionSettings.m_launchTimeout);
                        auto endConnectFromLaunchTime = AZStd::chrono::steady_clock::now();
                        if (!connectionEstablished && NegotiationWithAssetProcessorFailed())
                        {
                            AZ_Error(connectionSettings.m_connectionIdentifier.c_str(), false, "Negotiation with asset processor failed");
                        }
                        else
                        {
                            AZStd::chrono::duration<float> launchToConnectTime{ endConnectFromLaunchTime - startConnectFromLaunchTime };
                            if (connectionSettings.m_loggingCallback)
                            {
                                connectionSettings.m_loggingCallback(AZStd::fixed_string<128>::format("Launched Asset Processor and received connection in %f seconds\n",
                                    launchToConnectTime.count()));
                            }
                        }
                    }
                }
#endif
                while (!connectionEstablished && connectionSettings.m_waitForConnect)
                {
                    constexpr AZStd::chrono::seconds aSecond(1);
                    connectionEstablished = WaitUntilAssetProcessorConnected(aSecond);
                    if (!connectionEstablished && NegotiationWithAssetProcessorFailed())
                    {
                        AZ_Error(connectionSettings.m_connectionIdentifier.c_str(), false, "Negotiation with asset processor failed");
                        break;
                    }
                }
            }

            // If the wait until asset processor is ready option is unset
            // The return only whether a successful connection to the Asset Processor has taken place
            if (connectionEstablished && connectionSettings.m_waitUntilAssetProcessorIsReady)
            {
                // regardless of what is set in the bootstrap wait for AP to be ready
                // wait a maximum of 100 milliseconds and pump the system event loop until empty
                struct AssetsInQueueNotification
                    : public AzFramework::AssetSystemInfoBus::Handler
                {
                    AssetsInQueueNotification(const ConnectionSettings::LoggingCallback& callback)
                        : m_loggingCallback{ callback }
                    {
                    }
                    void CountOfAssetsInQueue(const int& count) override
                    {
                        // Pad to 7 digits as there should be any reasonable amount of jobs that are in the millions
                        // Carriage Return is used here to overwrite the current line of output
                        if (m_loggingCallback)
                        {
                            m_loggingCallback(AZStd::fixed_string<128>::format("Asset Processor working... %7d jobs remaining\r", count));
                        }
                    }

                    const ConnectionSettings::LoggingCallback& m_loggingCallback;
                };
                AssetsInQueueNotification assetsInQueueNotifcation(connectionSettings.m_loggingCallback);
                assetsInQueueNotifcation.BusConnect();

                if (connectionSettings.m_loggingCallback)
                {
                    connectionSettings.m_loggingCallback("Asset Processor working...\r");
                }

                bool assetProcessorIsReady = AssetProcessorIsReady() || WaitUntilAssetProcessorReady(connectionSettings.m_waitForReadyTimeout);
                assetsInQueueNotifcation.BusDisconnect();

                return connectionEstablished && assetProcessorIsReady;
            }

            return connectionEstablished;
        }

        bool AssetSystemComponent::WaitUntilAssetProcessorConnected(AZStd::chrono::duration<float> timeout)
        {
            AZStd::chrono::steady_clock::time_point start = AZStd::chrono::steady_clock::now();
            while (!ConnectedWithAssetProcessor() && AZStd::chrono::duration_cast<AZStd::chrono::milliseconds>(AZStd::chrono::steady_clock::now() - start) < timeout)
            {
                AssetSystemStatusBus::Broadcast(&AssetSystemStatusBus::Events::AssetSystemWaiting);

                if (NegotiationWithAssetProcessorFailed())
                {
                    AzFramework::AssetSystemConnectionNotificationsBus::Broadcast(
                        &AzFramework::AssetSystemConnectionNotificationsBus::Events::NegotiationFailed);
                    StartDisconnectingAssetProcessor();
                    return false;
                }

                //yield
                AZStd::this_thread::yield();
            }

            bool connected = ConnectedWithAssetProcessor();
            if ((!connected) && (m_socketConn->GetLastResult() != 0))
            {
                AZ_Warning("AssetProcessorConnection", false, "%s", m_socketConn->GetLastErrorMessage().c_str());
            }
            return connected;            
        }

        bool AssetSystemComponent::WaitUntilAssetProcessorReady(AZStd::chrono::duration<float> timeout)
        {
            if (!ConnectedWithAssetProcessor()) //don't wait if not connected
            {
                return false;
            }

            // while we wait, let's get some ping times.
            float pingTime = GetAssetProcessorPingTimeMilliseconds();
            if (pingTime > 0.0f)
            {
                AZ_TracePrintf("AssetSystem", "Ping time to asset processor: %0.2f milliseconds\n", pingTime);
            }

            AZStd::chrono::steady_clock::time_point start = AZStd::chrono::steady_clock::now();
            bool isAssetProcessorReady = false;
            while (!isAssetProcessorReady && (AZStd::chrono::steady_clock::now() - start) < timeout)
            {
                AzFramework::ApplicationRequests::Bus::Broadcast(&AzFramework::ApplicationRequests::PumpSystemEventLoopUntilEmpty);

                AssetSystemStatusBus::Broadcast(&AssetSystemStatusBus::Events::AssetSystemWaiting);

                if (!ConnectedWithAssetProcessor())
                {
                    //If we are here than it means we were connected but have lost connection with AP
                    AZ_Warning("AssetSystem", false, "Lost the connection to the Asset Processor!\nMake sure the Asset Processor is running.");
                    return false;
                }

                //Keep asking the AP about its status, until it is ready
                isAssetProcessorReady = AssetProcessorIsReady();
                if(!isAssetProcessorReady)
                {
                    // Throttle this, each loop actually sends network traffic to the AP and there's no point in running at 100x a second, but 10x is smooth.
                    AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(100)); // on some systems, PumpSystemEventLoopUntilEmpty may not sleep.
                }
            }

            return isAssetProcessorReady;
        }

        bool AssetSystemComponent::ConnectedWithAssetProcessor()
        {
            AZ_Assert(m_socketConn.get(), "SocketConnection doesn't exist!  Ensure AssetSystemComponent::Init was called");
            auto apConnection = azrtti_cast<AssetProcessorConnection*>(m_socketConn.get());
            return apConnection->IsConnected();
        }

        bool AssetSystemComponent::NegotiationWithAssetProcessorFailed()
        {
            AZ_Assert(m_socketConn.get(), "SocketConnection doesn't exist!  Ensure AssetSystemComponent::Init was called");
            auto apConnection = azrtti_cast<AssetProcessorConnection*>(m_socketConn.get());
            return apConnection->NegotiationFailed();
        }

        bool AssetSystemComponent::AssetProcessorIsReady()
        {
            if (!ConnectedWithAssetProcessor()) //cant be ready if not connected
            {
                return false;
            }

            RequestAssetProcessorStatus request;
            request.m_platform = m_assetProcessorPlatform;
            ResponseAssetProcessorStatus response;
            if (!SendRequest(request, response))
            {
                AZ_Warning("AssetSystem", false, "Failed to send Asset Processor Status request for platform %s.", m_assetProcessorPlatform.c_str());
                return false;
            }

            return response.m_isAssetProcessorReady;
        }

        void AssetSystemComponent::StartDisconnectingAssetProcessor()
        {
            AZ_Assert(m_socketConn.get(), "SocketConnection doesn't exist!  Ensure AssetSystemComponent::Init was called");
            auto apConnection = azrtti_cast<AssetProcessorConnection*>(m_socketConn.get());
            apConnection->Disconnect();
        }

        bool AssetSystemComponent::DisconnectedWithAssetProcessor()
        {
            AZ_Assert(m_socketConn.get(), "SocketConnection doesn't exist!  Ensure AssetSystemComponent::Init was called");
            auto apConnection = azrtti_cast<AssetProcessorConnection*>(m_socketConn.get());
            return apConnection->IsDisconnected();
        }

        bool AssetSystemComponent::WaitUntilAssetProcessorDisconnected(AZStd::chrono::duration<float> timeout)
        {
            AZStd::chrono::steady_clock::time_point start = AZStd::chrono::steady_clock::now();
            while (!DisconnectedWithAssetProcessor() && AZStd::chrono::duration_cast<AZStd::chrono::milliseconds>(AZStd::chrono::steady_clock::now() - start) < timeout)
            {
                //yield
                AZStd::this_thread::yield();
            }

            return DisconnectedWithAssetProcessor();
        }

        AssetStatus AssetSystemComponent::CompileAssetSync(const AZStd::string& assetPath)
        {
            return SendAssetStatusRequest(RequestAssetStatus(assetPath.c_str(), false, false));
        }

        AssetStatus AssetSystemComponent::CompileAssetSync_FlushIO(const AZStd::string& assetPath)
        {
            return SendAssetStatusRequest(RequestAssetStatus(assetPath.c_str(), false, true));
        }

        AssetStatus AssetSystemComponent::CompileAssetSyncById(const AZ::Data::AssetId& assetId)
        {
            return SendAssetStatusRequest(RequestAssetStatus(assetId, false, false));
        }

        AssetStatus AssetSystemComponent::CompileAssetSyncById_FlushIO(const AZ::Data::AssetId& assetId)
        {
            return SendAssetStatusRequest(RequestAssetStatus(assetId, false, true));
        }

        AssetStatus AssetSystemComponent::GetAssetStatus(const AZStd::string& assetPath)
        {
            return SendAssetStatusRequest(RequestAssetStatus(assetPath.c_str(), true, false));
        }

        AssetStatus AssetSystemComponent::GetAssetStatus_FlushIO(const AZStd::string& assetPath)
        {
            return SendAssetStatusRequest(RequestAssetStatus(assetPath.c_str(), true, true));
        }

        AssetStatus AssetSystemComponent::GetAssetStatusSearchType(const AZStd::string& assetPath, int searchType)
        {
            return SendAssetStatusRequest(RequestAssetStatus(assetPath.c_str(), true, false, searchType));
        }

        AssetStatus AssetSystemComponent::GetAssetStatusSearchType_FlushIO(const AZStd::string& assetPath, int searchType)
        {
            return SendAssetStatusRequest(RequestAssetStatus(assetPath.c_str(), true, true, searchType));
        }

        AssetStatus AssetSystemComponent::GetAssetStatusById(const AZ::Data::AssetId& assetId)
        {
            return SendAssetStatusRequest(RequestAssetStatus(assetId, true, false));
        }

        AssetStatus AssetSystemComponent::GetAssetStatusById_FlushIO(const AZ::Data::AssetId& assetId)
        {
            return SendAssetStatusRequest(RequestAssetStatus(assetId, true, true));
        }

        bool AssetSystemComponent::EscalateAssetByUuid(const AZ::Uuid& assetUuid)
        {
            if (ConnectedWithAssetProcessor())
            {
                RequestEscalateAsset request(assetUuid);
                SendRequest(request);
                return true;
            }

            return false; // not sent.
        }

        bool AssetSystemComponent::EscalateAssetBySearchTerm(AZStd::string_view searchTerm)
        {
            if (ConnectedWithAssetProcessor())
            {
                RequestEscalateAsset request(searchTerm.data());
                SendRequest(request);
                return true;
            }

            return false; // not sent.
        }

        void AssetSystemComponent::GetUnresolvedProductReferences(AZ::Data::AssetId assetId, AZ::u32& unresolvedAssetIdReferences, AZ::u32& unresolvedPathReferences)
        {
            AZ_Assert(m_socketConn.get(), "SocketConnection doesn't exist!  Ensure AssetSystemComponent::Init was called");

            unresolvedPathReferences = unresolvedAssetIdReferences = 0;

            if (ConnectedWithAssetProcessor())
            {
                GetUnresolvedDependencyCountsRequest request(assetId);
                GetUnresolvedDependencyCountsResponse response;

                if (SendRequest(request, response))
                {
                    unresolvedAssetIdReferences = response.m_unresolvedAssetIdReferences;
                    unresolvedPathReferences = response.m_unresolvedPathReferences;
                }
            }
        }

        AssetStatus AssetSystemComponent::SendAssetStatusRequest(const RequestAssetStatus& request)
        {
            AssetStatus eStatus = AssetStatus_Unknown;

            if (ConnectedWithAssetProcessor())
            {
                ResponseAssetStatus response;
                SendRequest(request, response);

                eStatus = static_cast<AssetStatus>(response.m_assetStatus);
            }

            return eStatus;
        }

        float AssetSystemComponent::GetAssetProcessorPingTimeMilliseconds()
        {
            if (!ConnectedWithAssetProcessor())
            {
                return 0.0f;
            }

            AZStd::chrono::steady_clock::time_point beforePing = AZStd::chrono::steady_clock::now();
            RequestPing pingeRequest;
            ResponsePing pingRespose;
            if (SendRequest(pingeRequest, pingRespose))
            {
                AZStd::chrono::duration<float, AZStd::milli> difference = AZStd::chrono::duration_cast<AZStd::chrono::duration<float, AZStd::milli> >(AZStd::chrono::steady_clock::now() - beforePing);
                return difference.count();
            }

            return 0.0f;
        }

        bool AssetSystemComponent::SaveCatalog()
        {
            if (!ConnectedWithAssetProcessor()) //cant be ready if not connected
            {
                return false;
            }

            SaveAssetCatalogRequest saveCatalogRequest;
            SaveAssetCatalogResponse saveCatalogRespose;
            if (SendRequest(saveCatalogRequest, saveCatalogRespose))
            {
                return saveCatalogRespose.m_saved;
            }

            return false;
        }
    } // namespace AssetSystem
} // namespace AzFramework
