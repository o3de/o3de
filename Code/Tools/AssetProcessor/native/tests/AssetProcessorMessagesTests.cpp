/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <native/FileWatcher/FileWatcher.h>
#include <utilities/BatchApplicationManager.h>
#include <utilities/ApplicationServer.h>
#include <AzFramework/Asset/AssetSystemComponent.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#if !defined(Q_MOC_RUN)
#include <AzCore/UnitTest/TestTypes.h>
#endif
#include <AzCore/Utils/Utils.h>
#include <connection/connectionManager.h>
#include <QCoreApplication>
#include <AzFramework/Network/AssetProcessorConnection.h>
#include <native/tests/MockAssetDatabaseRequestsHandler.h>

#include <gmock/gmock.h>

namespace AssetProcessorMessagesTests
{
    using namespace testing;

    using namespace AssetProcessor;
    using namespace AssetBuilderSDK;

    static constexpr unsigned short AssetProcessorPort{65535u};

    class AssetProcessorMessages;

    class MockFileWatcher;

    // a 'nice' mock doesn't complain if its methods are called without a prior 'expect_call'.
    using NiceMockFileWatcher = ::testing::NiceMock<MockFileWatcher>;
    class MockFileWatcher : public FileWatcherBase
    {
    public:
        MOCK_METHOD2(AddFolderWatch, void(QString, bool));
        MOCK_METHOD0(ClearFolderWatches, void());
        MOCK_METHOD0(StartWatching, void());
        MOCK_METHOD0(StopWatching, void());
        MOCK_METHOD2(InstallDefaultExclusionRules, void(QString, QString));
        MOCK_METHOD1(AddExclusion, void(const AssetBuilderSDK::FilePatternMatcher&));
        MOCK_CONST_METHOD1(IsExcluded, bool(QString));
    };

    struct UnitTestBatchApplicationManager
        : BatchApplicationManager
    {
        UnitTestBatchApplicationManager(int* argc, char*** argv, QObject* parent)
            : BatchApplicationManager(argc, argv, parent)
        {

        }

        void InitFileStateCache() override
        {
            m_fileStateCache = AZStd::make_unique<AssetProcessor::FileStatePassthrough>();
        }

        friend class AssetProcessorMessages;
    };

    struct MockAssetCatalog : AssetProcessor::AssetCatalog
    {
        MockAssetCatalog(QObject* parent, AssetProcessor::PlatformConfiguration* platformConfiguration)
            : AssetCatalog(parent, platformConfiguration)
        {
        }


        AzFramework::AssetSystem::GetUnresolvedDependencyCountsResponse HandleGetUnresolvedDependencyCountsRequest(MessageData<AzFramework::AssetSystem::GetUnresolvedDependencyCountsRequest> messageData) override
        {
            m_called = true;
            return AssetCatalog::HandleGetUnresolvedDependencyCountsRequest(messageData);
        }

        bool m_called = false;
    };

    struct MockAssetRequestHandler : AssetRequestHandler
    {
        bool InvokeHandler(MessageData<AzFramework::AssetSystem::BaseAssetProcessorMessage> message) override
        {
            m_invoked = true;

            return AssetRequestHandler::InvokeHandler(message);
        }

        // Mimic the necessary behavior in the standard AssetRequestHandler, so the event gets called.
        void OnNewIncomingRequest(unsigned int connId, unsigned int serial, QByteArray payload, QString platform) override
        {
            AZ::SerializeContext* serializeContext = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);
            AZStd::shared_ptr<AzFramework::AssetSystem::BaseAssetProcessorMessage> message{
                AZ::Utils::LoadObjectFromBuffer<AzFramework::AssetSystem::BaseAssetProcessorMessage>(
                    payload.constData(), payload.size(), serializeContext)
            };
            NetworkRequestID key(connId, serial);
            int fenceFileId = 0;
            m_pendingFenceRequestMap[fenceFileId] = AZStd::move(AssetRequestHandler::RequestInfo(key, AZStd::move(message), platform));

            OnFenceFileDetected(fenceFileId);
        }

        AZStd::atomic_bool m_invoked = false;
    };

    class AssetProcessorMessages
        : public ::UnitTest::LeakDetectionFixture
    {
    public:
        void SetUp() override
        {
            AssetUtilities::ResetGameName();

            m_dbConn.OpenDatabase();

            int argC = 0;
            m_batchApplicationManager = AZStd::make_unique<UnitTestBatchApplicationManager>(&argC, nullptr, nullptr);

            auto registry = AZ::SettingsRegistry::Get();
            EXPECT_NE(registry, nullptr);
            constexpr AZ::SettingsRegistryInterface::FixedValueString bootstrapKey{
                AZ::SettingsRegistryMergeUtils::BootstrapSettingsRootKey
            };
            constexpr AZ::SettingsRegistryInterface::FixedValueString projectPathKey{ bootstrapKey + "/project_path" };
            AZ::IO::FixedMaxPath enginePath;
            registry->Get(enginePath.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_EngineRootFolder);
            registry->Set(projectPathKey, (enginePath / "AutomatedTesting").Native());
            AZ::SettingsRegistryMergeUtils::MergeSettingsToRegistry_AddRuntimeFilePaths(*registry);

            // Force the branch token into settings registry before starting the application manager.
            // This avoids writing the asset_processor.setreg file which can cause fileIO errors.
            constexpr AZ::SettingsRegistryInterface::FixedValueString branchTokenKey{ bootstrapKey + "/assetProcessor_branch_token" };
            AZStd::string token;
            AZ::StringFunc::AssetPath::CalculateBranchToken(enginePath.c_str(), token);
            registry->Set(branchTokenKey, token.c_str());

            auto status = m_batchApplicationManager->BeforeRun();
            ASSERT_EQ(status, ApplicationManager::BeforeRunStatus::Status_Success);

            m_batchApplicationManager->m_platformConfiguration = new PlatformConfiguration();

            AZStd::vector<ApplicationManagerBase::APCommandLineSwitch> commandLineInfo;
            m_batchApplicationManager->InitAssetProcessorManager(commandLineInfo);

            m_assetCatalog = AZStd::make_unique<MockAssetCatalog>(nullptr, m_batchApplicationManager->m_platformConfiguration);

            m_batchApplicationManager->m_assetCatalog = m_assetCatalog.get();
            m_batchApplicationManager->InitRCController();
            m_batchApplicationManager->InitFileStateCache();
            m_batchApplicationManager->InitFileMonitor(AZStd::make_unique<NiceMockFileWatcher>());
            m_batchApplicationManager->InitApplicationServer();
            m_batchApplicationManager->InitConnectionManager();
            // Note this must be constructed after InitConnectionManager is called since it will interact with the connection manager
            m_assetRequestHandler = new MockAssetRequestHandler();
            m_batchApplicationManager->InitAssetRequestHandler(m_assetRequestHandler);
            m_batchApplicationManager->ConnectAssetCatalog();

            QObject::connect(m_batchApplicationManager->m_connectionManager, &ConnectionManager::ConnectionError, [](unsigned /*connId*/, QString error)
                {
                    AZ_Error("ConnectionManager", false, "%s", error.toUtf8().constData());
                });

            ASSERT_TRUE(m_batchApplicationManager->m_applicationServer->startListening(AssetProcessorPort));

            using namespace AzFramework;

            m_assetSystemComponent = AZStd::make_unique<AssetSystem::AssetSystemComponent>();

            m_assetSystemComponent->Init();
            m_assetSystemComponent->Activate();

            QCoreApplication::processEvents();

            RunNetworkRequest([]()
                {
                    AZStd::string appBranchToken;
                    AzFramework::ApplicationRequests::Bus::Broadcast(&AzFramework::ApplicationRequests::CalculateBranchTokenForEngineRoot, appBranchToken);

                    AzFramework::AssetSystem::ConnectionSettings connectionSettings;
                    connectionSettings.m_assetProcessorIp = "127.0.0.1";
                    connectionSettings.m_assetProcessorPort = AssetProcessorPort;
                    connectionSettings.m_branchToken = appBranchToken;
                    connectionSettings.m_projectName = "AutomatedTesting";
                    connectionSettings.m_assetPlatform = "pc";
                    connectionSettings.m_connectionIdentifier = "UNITTEST";
                    connectionSettings.m_connectTimeout = AZStd::chrono::seconds(15);
                    connectionSettings.m_connectionDirection = AzFramework::AssetSystem::ConnectionSettings::ConnectionDirection::ConnectToAssetProcessor;
                    connectionSettings.m_waitUntilAssetProcessorIsReady = false;
                    connectionSettings.m_launchAssetProcessorOnFailedConnection = false;

                    bool result = false;
                    AzFramework::AssetSystemRequestBus::BroadcastResult(result,
                        &AzFramework::AssetSystemRequestBus::Events::EstablishAssetProcessorConnection, connectionSettings);

                    ASSERT_TRUE(result);
                });
        }

        void TearDown() override
        {
            if (m_batchApplicationManager->m_connectionManager)
            {
                QEventLoop eventLoop;

                QObject::connect(m_batchApplicationManager->m_connectionManager, &ConnectionManager::ReadyToQuit, &eventLoop, &QEventLoop::quit);

                m_batchApplicationManager->m_connectionManager->QuitRequested();

                eventLoop.exec();
            }

            if (m_assetSystemComponent)
            {
                m_assetSystemComponent->Deactivate();
            }
            m_batchApplicationManager->Destroy();

            m_assetCatalog.reset();
            m_assetSystemComponent.reset();
            m_batchApplicationManager.reset();
        }

        void RunNetworkRequest(AZStd::function<void()> func) const
        {
            AZStd::atomic_bool finished = false;
            auto start = AZStd::chrono::steady_clock::now();

            auto thread = AZStd::thread({/*m_name =*/ "MessageTests"}, [&finished, &func]()
                {
                    func();
                    finished = true;
                }
            );

            constexpr int MaxWaitTime = 5;
            while (!finished && AZStd::chrono::steady_clock::now() - start < AZStd::chrono::seconds(MaxWaitTime))
            {
                QCoreApplication::processEvents();
            }

            ASSERT_TRUE(finished) << "Timeout";

            thread.join();
        }

    protected:

        MockAssetRequestHandler* m_assetRequestHandler{}; // Not owned, AP will delete this pointer
        AZStd::unique_ptr<UnitTestBatchApplicationManager> m_batchApplicationManager;
        AZStd::unique_ptr<AzFramework::AssetSystem::AssetSystemComponent> m_assetSystemComponent;
        AssetProcessor::MockAssetDatabaseRequestsHandler m_databaseLocationListener;
        AZStd::unique_ptr<MockAssetCatalog> m_assetCatalog = nullptr;
        AZStd::string m_databaseLocation;
        AssetDatabaseConnection m_dbConn;
    };

    struct MessagePair
    {
        AZStd::unique_ptr<AzFramework::AssetSystem::BaseAssetProcessorMessage> m_request;
        AZStd::unique_ptr<AzFramework::AssetSystem::BaseAssetProcessorMessage> m_response;
    };

    TEST_F(AssetProcessorMessages, All)
    {
        // Test that we can successfully send network messages and have them arrive for processing
        // For messages that have a response, it also verifies the response comes back
        // Note that several harmless warnings will be triggered due to the messages not having any data set
        using namespace AzFramework::AssetSystem;
        using namespace AzToolsFramework::AssetSystem;

        AZStd::vector<MessagePair> testMessages;
        AZStd::unordered_map<int, AZStd::string> nameMap; // This is just for debugging, so we can output the name of failed messages

        AZ::SerializeContext* serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);

        auto addPairFunc = [&testMessages, &nameMap, serializeContext](auto* request, auto* response)
        {
            testMessages.emplace_back(MessagePair{
                AZStd::unique_ptr<AZStd::remove_pointer_t<decltype(request)>>(request),
                AZStd::unique_ptr<AZStd::remove_pointer_t<decltype(response)>>(response)
            });

            auto data = serializeContext->FindClassData(request->RTTI_GetType());
            nameMap[request->GetMessageType()] = data->m_name;
        };

        auto addRequestFunc = [&testMessages, &nameMap, serializeContext](auto* request)
        {
            testMessages.emplace_back(MessagePair{AZStd::unique_ptr<AZStd::remove_pointer_t<decltype(request)>>(request), nullptr });

            auto data = serializeContext->FindClassData(request->RTTI_GetType());
            nameMap[request->GetMessageType()] = data->m_name;
        };

        addPairFunc(new GetFullSourcePathFromRelativeProductPathRequest(), new GetFullSourcePathFromRelativeProductPathResponse());
        addPairFunc(new GetRelativeProductPathFromFullSourceOrProductPathRequest(), new GetRelativeProductPathFromFullSourceOrProductPathResponse());
        addPairFunc(
            new GenerateRelativeSourcePathRequest(),
            new GenerateRelativeSourcePathResponse());
        addPairFunc(new SourceAssetInfoRequest(), new SourceAssetInfoResponse());
        addPairFunc(new SourceAssetProductsInfoRequest(), new SourceAssetProductsInfoResponse());
        addPairFunc(new GetScanFoldersRequest(), new GetScanFoldersResponse());
        addPairFunc(new GetAssetSafeFoldersRequest(), new GetAssetSafeFoldersResponse());
        addRequestFunc(new RegisterSourceAssetRequest());
        addRequestFunc(new UnregisterSourceAssetRequest());
        addPairFunc(new AssetInfoRequest(), new AssetInfoResponse());
        addPairFunc(new AssetDependencyInfoRequest(), new AssetDependencyInfoResponse());
        addRequestFunc(new RequestEscalateAsset());
        addPairFunc(new RequestAssetStatus(), new ResponseAssetStatus());
        addPairFunc(new AssetFingerprintClearRequest(), new AssetFingerprintClearResponse());

        RunNetworkRequest([&testMessages, &nameMap, this]()
            {
                for(auto&& pair : testMessages)
                {
                    AZStd::string messageName = nameMap[pair.m_request->GetMessageType()];

                    m_assetRequestHandler->m_invoked = false;

                    if(pair.m_response)
                    {
                        EXPECT_TRUE(SendRequest(*pair.m_request.get(), *pair.m_response.get())) << "Message " << messageName.c_str() << " failed to send";
                    }
                    else
                    {
                        EXPECT_TRUE(SendRequest(*pair.m_request.get())) << "Message " << messageName.c_str() << " failed to send";
                    }

                    // Even if there is a response, the send request may finish before the response finishes, so wait a few seconds to see if the message has sent.
                    // This exits early if the message is invoked.
                    constexpr int MaxWaitTimeSeconds = 5;
                    auto start = AZStd::chrono::steady_clock::now();

                    while (!m_assetRequestHandler->m_invoked &&
                           AZStd::chrono::steady_clock::now() - start < AZStd::chrono::seconds(MaxWaitTimeSeconds))
                    {
                        AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(10));
                    }

                    EXPECT_TRUE(m_assetRequestHandler->m_invoked) << "Message " << messageName.c_str() << " was not received";
                }
            });
    }

    TEST_F(AssetProcessorMessages, GetUnresolvedProductReferences_Succeeds)
    {
        using namespace AzToolsFramework::AssetDatabase;

        // Setup the database with all needed info
        ScanFolderDatabaseEntry scanfolder1("scanfolder1", "scanfolder1", "scanfolder1");
        ASSERT_TRUE(m_dbConn.SetScanFolder(scanfolder1));

        SourceDatabaseEntry source1(scanfolder1.m_scanFolderID, "source1.png", AZ::Uuid::CreateRandom(), "Fingerprint");
        SourceDatabaseEntry source2(scanfolder1.m_scanFolderID, "source2.png", AZ::Uuid::CreateRandom(), "Fingerprint");
        ASSERT_TRUE(m_dbConn.SetSource(source1));
        ASSERT_TRUE(m_dbConn.SetSource(source2));

        JobDatabaseEntry job1(source1.m_sourceID, "jobkey", 1234, "pc", AZ::Uuid::CreateRandom(), AzToolsFramework::AssetSystem::JobStatus::Completed, 1111);
        JobDatabaseEntry job2(source2.m_sourceID, "jobkey", 1234, "pc", AZ::Uuid::CreateRandom(), AzToolsFramework::AssetSystem::JobStatus::Completed, 2222);
        ASSERT_TRUE(m_dbConn.SetJob(job1));
        ASSERT_TRUE(m_dbConn.SetJob(job2));

        ProductDatabaseEntry product1(job1.m_jobID, 5, "source1.product", AZ::Data::AssetType::CreateRandom());
        ProductDatabaseEntry product2(job2.m_jobID, 15, "source2.product", AZ::Data::AssetType::CreateRandom());
        ASSERT_TRUE(m_dbConn.SetProduct(product1));
        ASSERT_TRUE(m_dbConn.SetProduct(product2));

        ProductDependencyDatabaseEntry dependency1(product1.m_productID, AZ::Uuid::CreateNull(), 0, {}, "pc", 0, "somefileA.txt", ProductDependencyDatabaseEntry::DependencyType::ProductDep_SourceFile);
        ProductDependencyDatabaseEntry dependency2(product1.m_productID, AZ::Uuid::CreateNull(), 0, {}, "pc", 0, "somefileB.txt", ProductDependencyDatabaseEntry::DependencyType::ProductDep_ProductFile);
        ProductDependencyDatabaseEntry dependency3(product1.m_productID, AZ::Uuid::CreateNull(), 0, {}, "pc", 0, "somefileC.txt");
        ProductDependencyDatabaseEntry dependency4(product1.m_productID, AZ::Uuid::CreateNull(), 0, {}, "pc", 0, ":somefileD.txt"); // Exclusion
        ProductDependencyDatabaseEntry dependency5(product1.m_productID, AZ::Uuid::CreateNull(), 0, {}, "pc", 0, "somefileE*.txt"); // Wildcard
        ASSERT_TRUE(m_dbConn.SetProductDependency(dependency1));
        ASSERT_TRUE(m_dbConn.SetProductDependency(dependency2));
        ASSERT_TRUE(m_dbConn.SetProductDependency(dependency3));
        ASSERT_TRUE(m_dbConn.SetProductDependency(dependency4));
        ASSERT_TRUE(m_dbConn.SetProductDependency(dependency5));

        // Setup the asset catalog
        AzFramework::AssetSystem::AssetNotificationMessage assetNotificationMessage("source1.product", AzFramework::AssetSystem::AssetNotificationMessage::NotificationType::AssetChanged, AZ::Data::AssetType::CreateRandom(), "pc");
        assetNotificationMessage.m_assetId = AZ::Data::AssetId(source1.m_sourceGuid, product1.m_subID);
        assetNotificationMessage.m_dependencies.push_back(AZ::Data::ProductDependency(AZ::Data::AssetId(source2.m_sourceGuid, product2.m_subID), {}));

        m_assetCatalog->OnAssetMessage(assetNotificationMessage);

        // Run the actual test
        RunNetworkRequest([&source1, &product1]()
            {
                using namespace AzFramework;

                AZ::u32 assetReferenceCount, pathReferenceCount;
                AZ::Data::AssetId assetId = AZ::Data::AssetId(source1.m_sourceGuid, product1.m_subID);
                AssetSystemRequestBus::Broadcast(&AssetSystemRequestBus::Events::GetUnresolvedProductReferences, assetId, assetReferenceCount, pathReferenceCount);

                ASSERT_EQ(assetReferenceCount, 1);
                ASSERT_EQ(pathReferenceCount, 3);
            });

        ASSERT_TRUE(m_assetCatalog->m_called);
    }
}
