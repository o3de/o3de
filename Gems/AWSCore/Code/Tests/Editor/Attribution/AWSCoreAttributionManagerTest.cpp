/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Editor/Attribution/AWSCoreAttributionManager.h>
#include <Editor/Attribution/AWSCoreAttributionMetric.h>
#include <Credential/AWSCredentialBus.h>

#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/base.h>
#include <AzCore/Settings/SettingsRegistry.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/Serialization/Json/JsonSystemComponent.h>
#include <AzCore/Serialization/Json/RegistrationContext.h>
#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Jobs/JobManager.h>
#include <AzCore/Jobs/JobManagerBus.h>
#include <AzCore/Jobs/JobContext.h>
#include <AzCore/Utils/Utils.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Module/ModuleManagerBus.h>
#include <AzCore/PlatformId/PlatformId.h>
#include <AzTest/Utils.h>

#include <TestFramework/AWSCoreFixture.h>
#include <QSysInfo>
#include <QString>


using namespace AWSCore;

namespace AWSAttributionUnitTest
{
    class ModuleDataMock:
        public AZ::ModuleData
    {
    public:
        AZStd::shared_ptr<AZ::Entity> m_entity;
        ModuleDataMock(AZStd::string name)
        {
            m_entity = AZStd::make_shared<AZ::Entity>();
            m_entity->SetName(name);
        }

        virtual ~ModuleDataMock()
        {
            m_entity.reset();
        }

        AZ::DynamicModuleHandle* GetDynamicModuleHandle() const override
        {
            return nullptr;
        }

        /// Get the handle to the module class
        AZ::Module* GetModule() const override
        {
            return nullptr;
        }

        /// Get the entity this module uses as a System Entity
        AZ::Entity* GetEntity() const override
        {
            return m_entity.get();
        }

        /// Get the debug name of the module
        const char* GetDebugName() const override
        {
            return m_entity->GetName().c_str();
        }
    };

    class ModuleManagerRequestBusMock
        : public AZ::ModuleManagerRequestBus::Handler
    {
    public:

        void EnumerateModulesMock(AZ::ModuleManagerRequests::EnumerateModulesCallback perModuleCallback)
        {
            auto data = ModuleDataMock("AWSCore.Editor.dll");
            perModuleCallback(data);
            data = ModuleDataMock("AWSClientAuth.so");
            perModuleCallback(data);
        }

        ModuleManagerRequestBusMock()
        {
            AZ::ModuleManagerRequestBus::Handler::BusConnect();
            ON_CALL(*this, EnumerateModules(testing::_)).WillByDefault(testing::Invoke(this, &ModuleManagerRequestBusMock::EnumerateModulesMock));
        }

        ~ModuleManagerRequestBusMock()
        {
            AZ::ModuleManagerRequestBus::Handler::BusDisconnect();
        }

        MOCK_METHOD1(EnumerateModules, void(AZ::ModuleManagerRequests::EnumerateModulesCallback perModuleCallback));
        MOCK_METHOD3(LoadDynamicModule, AZ::ModuleManagerRequests::LoadModuleOutcome(const char* modulePath, AZ::ModuleInitializationSteps lastStepToPerform, bool maintainReference));
        MOCK_METHOD3(LoadDynamicModules, AZ::ModuleManagerRequests::LoadModulesResult(const AZ::ModuleDescriptorList& modules, AZ::ModuleInitializationSteps lastStepToPerform, bool maintainReferences));
        MOCK_METHOD2(LoadStaticModules, AZ::ModuleManagerRequests::LoadModulesResult(AZ::CreateStaticModulesCallback staticModulesCb, AZ::ModuleInitializationSteps lastStepToPerform));
        MOCK_METHOD1(IsModuleLoaded, bool(const char* modulePath));
    };

    class AWSCredentialRquestsBusMock
        : public AWSCore::AWSCredentialRequestBus::Handler
    {
    public:
        AWSCredentialRquestsBusMock()
        {
            m_provider = std::make_shared<Aws::Auth::SimpleAWSCredentialsProvider>("TestAccessKey", "TestSecreKey", "TestSession");
            AWSCore::AWSCredentialRequestBus::Handler::BusConnect();
            ON_CALL(*this, GetCredentialsProvider()).WillByDefault(testing::Return(m_provider));
            ON_CALL(*this, GetCredentialHandlerOrder()).WillByDefault(testing::Return(CredentialHandlerOrder::DEFAULT_CREDENTIAL_HANDLER));
        }

        ~AWSCredentialRquestsBusMock()
        {
            AWSCore::AWSCredentialRequestBus::Handler::BusDisconnect();
            m_provider.reset();
        }

        MOCK_CONST_METHOD0(GetCredentialHandlerOrder, int());
        MOCK_METHOD0(GetCredentialsProvider, std::shared_ptr<Aws::Auth::AWSCredentialsProvider>());

        std::shared_ptr<Aws::Auth::AWSCredentialsProvider> m_provider;
    };

    class AWSAttributionManagerMock
        : public AWSAttributionManager
    {
    public:
        using AWSAttributionManager::SubmitMetric;
        using AWSAttributionManager::UpdateMetric;
        using AWSAttributionManager::SetApiEndpointAndRegion;
        using AWSAttributionManager::ShowConsentDialog;

        AWSAttributionManagerMock()
        {
            ON_CALL(*this, SubmitMetric(testing::_)).WillByDefault(testing::Invoke(this, &AWSAttributionManagerMock::SubmitMetricMock));
        }

        MOCK_METHOD1(SubmitMetric, void(AttributionMetric& metric));
        MOCK_METHOD0(ShowConsentDialog, void());

        void SubmitMetricMock(AttributionMetric& metric)
        {
            AZ_UNUSED(metric);
            UpdateLastSend();
        }
    };

    class AttributionManagerTest
        : public AWSCoreFixture
    {
    public:

        virtual ~AttributionManagerTest() = default;

        // List of expected platform values 
        const AZStd::vector<AZStd::string> m_validPlatformValues={"PC", "Linux", "Mac"};

    protected:
        AZStd::shared_ptr<AZ::SerializeContext> m_serializeContext;
        AZStd::unique_ptr<AZ::JsonRegistrationContext> m_registrationContext;
        AZStd::unique_ptr<AZ::JobContext> m_jobContext;
        AZStd::unique_ptr<AZ::JobCancelGroup> m_jobCancelGroup;
        AZStd::unique_ptr<AZ::JobManager> m_jobManager;
        AZStd::array<char, AZ::IO::MaxPathLength> m_resolvedSettingsPath;
        ModuleManagerRequestBusMock m_moduleManagerRequestBusMock;
        AZ::Test::ScopedAutoTempDirectory m_tempDirectory;

        void SetUp() override
        {
            AWSCoreFixture::SetUp();

            char rootPath[AZ_MAX_PATH_LEN];
            AZ::Utils::GetExecutableDirectory(rootPath, AZ_MAX_PATH_LEN);
            m_localFileIO->SetAlias("@user@", m_tempDirectory.GetDirectory());

            m_localFileIO->ResolvePath("@user@/Registry/", m_resolvedSettingsPath.data(), m_resolvedSettingsPath.size());
            AZ::IO::SystemFile::CreateDir(m_resolvedSettingsPath.data());

            m_localFileIO->ResolvePath("@user@/Registry/editor_aws_preferences.setreg", m_resolvedSettingsPath.data(), m_resolvedSettingsPath.size());

            m_serializeContext = AZStd::make_unique<AZ::SerializeContext>();

            AZ::JsonSystemComponent::Reflect(m_registrationContext.get());

            m_settingsRegistry->SetContext(m_serializeContext.get());
            m_settingsRegistry->SetContext(m_registrationContext.get());

            AZ::JobManagerDesc jobManagerDesc;
            AZ::JobManagerThreadDesc threadDesc;

            m_jobManager.reset(aznew AZ::JobManager(jobManagerDesc));
            m_jobCancelGroup.reset(aznew AZ::JobCancelGroup());
            jobManagerDesc.m_workerThreads.push_back(threadDesc);
            m_jobContext.reset(aznew AZ::JobContext(*m_jobManager, *m_jobCancelGroup));
            AZ::JobContext::SetGlobalContext(m_jobContext.get());
        }

        void TearDown() override
        {
            AZ::JobContext::SetGlobalContext(nullptr);
            m_jobContext.reset();
            m_jobCancelGroup.reset();
            m_jobManager.reset();

            m_serializeContext.reset();
            m_registrationContext.reset();

            m_localFileIO->ResolvePath("@user@/Registry/", m_resolvedSettingsPath.data(), m_resolvedSettingsPath.size());
            AZ::IO::SystemFile::DeleteDir(m_resolvedSettingsPath.data());

            AZ::IO::FileIOBase::SetInstance(nullptr);

            AWSCoreFixture::TearDown();
        }
    };

    TEST_F(AttributionManagerTest, MetricsSettings_ConsentShown_AttributionDisabled_SkipsSend)
    {
        // GIVEN
        AWSCredentialRquestsBusMock credentialRequestBusMock;
        AWSAttributionManagerMock manager;
       
        CreateFile(m_resolvedSettingsPath.data(), R"({
            "Amazon": {
                "AWS": {
                    "Preferences": {
                        "AWSAttributionConsentShown": true,
                        "AWSAttributionEnabled": false,
                        "AWSAttributionDelaySeconds": 30
                    }
                }
            }
        })");

        manager.Init();

        EXPECT_CALL(manager, SubmitMetric(testing::_)).Times(0);
        EXPECT_CALL(m_moduleManagerRequestBusMock, EnumerateModules(testing::_)).Times(0);
        EXPECT_CALL(credentialRequestBusMock, GetCredentialsProvider()).Times(1);

        // WHEN
        manager.MetricCheck();

        // THEN
        AZ::u64 timeStamp = 0;
        m_settingsRegistry->Get(timeStamp, "/Amazon/AWS/Preferences/AWSAttributionLastTimeStamp");
        ASSERT_TRUE(timeStamp == 0);

        RemoveFile(m_resolvedSettingsPath.data());
    }

    TEST_F(AttributionManagerTest, AttributionEnabled_ContentShown_NoPreviousTimeStamp_SendSuccess)
    {
        // GIVEN
        AWSCredentialRquestsBusMock credentialRequestBusMock;
        AWSAttributionManagerMock manager;

        CreateFile(m_resolvedSettingsPath.data(), R"({
            "Amazon": {
                "AWS": {
                    "Preferences": {
                        "AWSAttributionConsentShown": true,
                        "AWSAttributionEnabled": true,
                        "AWSAttributionDelaySeconds": 30,
                    }
                }
            }
        })");
        manager.Init();

        EXPECT_CALL(manager, SubmitMetric(testing::_)).Times(1);
        EXPECT_CALL(m_moduleManagerRequestBusMock, EnumerateModules(testing::_)).Times(1);
        EXPECT_CALL(credentialRequestBusMock, GetCredentialsProvider()).Times(1);

        // WHEN
        manager.MetricCheck();

        // THEN
        AZ::u64 timeStamp = 0;
        m_settingsRegistry->Get(timeStamp, "/Amazon/AWS/Preferences/AWSAttributionLastTimeStamp");
        ASSERT_TRUE(timeStamp > 0);


        RemoveFile(m_resolvedSettingsPath.data());
    }

    TEST_F(AttributionManagerTest, AttributionEnabled_ContentShown_ValidPreviousTimeStamp_SendSuccess)
    {
        // GIVEN
        AWSCredentialRquestsBusMock credentialRequestBusMock;
        AWSAttributionManagerMock manager;

        CreateFile(m_resolvedSettingsPath.data(), R"({
            "Amazon": {
                "AWS": {
                    "Preferences": {
                        "AWSAttributionConsentShown": true,
                        "AWSAttributionEnabled": true,
                        "AWSAttributionDelaySeconds": 30,
                        "AWSAttributionLastTimeStamp": 629400
                    }
                }
            }
        })");

        manager.Init();

        EXPECT_CALL(manager, SubmitMetric(testing::_)).Times(1);
        EXPECT_CALL(m_moduleManagerRequestBusMock, EnumerateModules(testing::_)).Times(1);
        EXPECT_CALL(credentialRequestBusMock, GetCredentialsProvider()).Times(1);

        // WHEN
        manager.MetricCheck();

        // THEN
        AZ::u64 timeStamp = 0;
        m_settingsRegistry->Get(timeStamp, "/Amazon/AWS/Preferences/AWSAttributionLastTimeStamp");
        ASSERT_TRUE(timeStamp > 0);

        RemoveFile(m_resolvedSettingsPath.data());
    }

    TEST_F(AttributionManagerTest, AttributionEnabled_ContentShown_DelayNotSatisfied_SendFail)
    {
        // GIVEN
        AWSCredentialRquestsBusMock credentialRequestBusMock;
        AWSAttributionManagerMock manager;

        CreateFile(m_resolvedSettingsPath.data(), R"({
            "Amazon": {
                "AWS": {
                    "Preferences": {
                        "AWSAttributionConsentShown": true,
                        "AWSAttributionEnabled": true,
                        "AWSAttributionDelaySeconds": 300,
                        "AWSAttributionLastTimeStamp": 0
                    }
                }
            }
        })");

        manager.Init();

        AZ::u64 delayInSeconds = AZStd::chrono::duration_cast<AZStd::chrono::seconds>(AZStd::chrono::system_clock::now().time_since_epoch()).count();
        ASSERT_TRUE(m_settingsRegistry->Set("/Amazon/AWS/Preferences/AWSAttributionLastTimeStamp", delayInSeconds));

        EXPECT_CALL(manager, SubmitMetric(testing::_)).Times(0);
        EXPECT_CALL(m_moduleManagerRequestBusMock, EnumerateModules(testing::_)).Times(0);
        EXPECT_CALL(credentialRequestBusMock, GetCredentialsProvider()).Times(1);

        // WHEN
        manager.MetricCheck();

        // THEN
        AZ::u64 timeStamp = 0;
        m_settingsRegistry->Get(timeStamp, "/Amazon/AWS/Preferences/AWSAttributionLastTimeStamp");
        ASSERT_TRUE(timeStamp == delayInSeconds);

        RemoveFile(m_resolvedSettingsPath.data());
    }

    TEST_F(AttributionManagerTest, AttributionEnabledNotFound_ContentShown_SendFail)
    {
        // GIVEN
        AWSCredentialRquestsBusMock credentialRequestBusMock;
        AWSAttributionManagerMock manager;

        CreateFile(m_resolvedSettingsPath.data(), R"({
            "Amazon": {
                "AWS": {
                    "Preferences": {
                        "AWSAttributionConsentShown": true
                    }
                }
            }
        })");

        manager.Init();

        EXPECT_CALL(manager, SubmitMetric(testing::_)).Times(0);
        EXPECT_CALL(m_moduleManagerRequestBusMock, EnumerateModules(testing::_)).Times(0);
        EXPECT_CALL(credentialRequestBusMock, GetCredentialsProvider()).Times(1);

        // WHEN
        manager.MetricCheck();

        // THEN
        AZ::u64 timeStamp = 0;
        m_settingsRegistry->Get(timeStamp, "/Amazon/AWS/Preferences/AWSAttributionLastTimeStamp");
        ASSERT_TRUE(timeStamp == 0);

        RemoveFile(m_resolvedSettingsPath.data());
    }

    TEST_F(AttributionManagerTest, AttributionEnabledNotFound_ContentNotShown_SendFail)
    {
        // GIVEN
        AWSCredentialRquestsBusMock credentialRequestBusMock;
        AWSAttributionManagerMock manager;
        manager.Init();

        EXPECT_CALL(manager, SubmitMetric(testing::_)).Times(0);
        EXPECT_CALL(m_moduleManagerRequestBusMock, EnumerateModules(testing::_)).Times(0);
        EXPECT_CALL(credentialRequestBusMock, GetCredentialsProvider()).Times(1);
        EXPECT_CALL(manager, ShowConsentDialog()).Times(1);

        // WHEN
        manager.MetricCheck();

        // THEN
        ASSERT_FALSE(m_localFileIO->Exists(m_resolvedSettingsPath.data()));
    }

    TEST_F(AttributionManagerTest, SetApiEndpointAndRegion_Success)
    {
        // GIVEN
        AWSAttributionManagerMock manager;
        AWSCore::ServiceAPI::AWSAttributionRequestJob::Config* config = aznew AWSCore::ServiceAPI::AWSAttributionRequestJob::Config();

        // WHEN
        manager.SetApiEndpointAndRegion(config);

        // THEN
        ASSERT_TRUE(config->region  == Aws::Region::US_EAST_1);
        ASSERT_TRUE(config->endpointOverride->find("o3deattribution.us-east-1.amazonaws.com") != Aws::String::npos);

        delete config;
    }

    TEST_F(AttributionManagerTest, UpdateMetric_Success)
    {
        // GIVEN
        AWSAttributionManagerMock manager;
        AttributionMetric metric;
        AZStd::string expectedPlatform = AWSAttributionManager::MapPlatform(AZ::g_currentPlatform);
        AZStd::array<char, AZ::IO::MaxPathLength> engineJsonPath;
        m_localFileIO->ResolvePath("@user@/Registry/engine.json", engineJsonPath.data(), engineJsonPath.size());
        CreateFile(engineJsonPath.data(), R"({"O3DEVersion": "1.0.0.0"})");

        m_localFileIO->ResolvePath("@user@/Registry/", engineJsonPath.data(), engineJsonPath.size());
        m_settingsRegistry->Set(AZ::SettingsRegistryMergeUtils::FilePathKey_EngineRootFolder, engineJsonPath.data());

        EXPECT_CALL(m_moduleManagerRequestBusMock, EnumerateModules(testing::_)).Times(1);

        // WHEN
        manager.UpdateMetric(metric);

        // THEN
        AZStd::string serializedMetricValue = metric.SerializeToJson();
        ASSERT_TRUE(serializedMetricValue.find("\"o3de_version\":\"1.0.0.0\"") != AZStd::string::npos);
        const auto platformValue = serializedMetricValue.find(expectedPlatform);
        ASSERT_NE(platformValue, AZStd::string::npos);
        EXPECT_NE(AZStd::find(m_validPlatformValues.begin(), m_validPlatformValues.end(), metric.GetPlatform()), m_validPlatformValues.end());

        ASSERT_TRUE(serializedMetricValue.find(QSysInfo::prettyProductName().toStdString().c_str()) != AZStd::string::npos);
        ASSERT_TRUE(serializedMetricValue.find("AWSCore.Editor") != AZStd::string::npos);
        ASSERT_TRUE(serializedMetricValue.find("AWSClientAuth") != AZStd::string::npos);

        RemoveFile(engineJsonPath.data());
    }

} // namespace AWSCoreUnitTest
