/*
 * All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
 * its licensors.
 *
 * For complete copyright and license terms please see the LICENSE at the root of this
 * distribution (the "License"). All use of this software is governed by the License,
 * or, if provided, by the license below or the license accompanying this file. Do not
 * remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *
 */

#include <Editor/Attribution/AWSCoreAttributionManager.h>
#include <Editor/Attribution/AWSCoreAttributionMetric.h>

#include <AzFramework/IO/LocalFileIO.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/base.h>
#include <AzCore/Settings/SettingsRegistry.h>
#include <AzCore/Settings/SettingsRegistryImpl.h>
#include <AzCore/Serialization/Json/JsonSystemComponent.h>
#include <AzCore/Serialization/Json/RegistrationContext.h>
#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Jobs/JobManager.h>
#include <AzCore/Jobs/JobManagerBus.h>
#include <AzCore/Jobs/JobContext.h>
#include <AzCore/Utils/Utils.h>
#include <AzCore/UnitTest/TestTypes.h>

#include <TestFramework/AWSCoreFixture.h>


using namespace AWSCore;

namespace AWSAttributionUnitTest
{

    class AWSAttributionManagerMock
        : public AWSAttributionManager
    {
    public:

        AWSAttributionManagerMock()
        {
            ON_CALL(*this, SubmitMetric(testing::_)).WillByDefault(testing::Invoke(this, &AWSAttributionManagerMock::SubmitMetricMock));
        }
        using AWSAttributionManager::SubmitMetric;

        MOCK_METHOD1(SubmitMetric, void(AttributionMetric& metric));

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

    protected:
        AZStd::shared_ptr<AZ::SerializeContext> m_serializeContext;
        AZStd::unique_ptr<AZ::JsonRegistrationContext> m_registrationContext;
        AZStd::shared_ptr<AZ::SettingsRegistryImpl> m_settingsRegistry;
        AZStd::unique_ptr<AZ::JobContext> m_jobContext;
        AZStd::unique_ptr<AZ::JobCancelGroup> m_jobCancelGroup;
        AZStd::unique_ptr<AZ::JobManager> m_jobManager;
        AZStd::array<char, AZ::IO::MaxPathLength> m_resolvedSettingsPath;

        void SetUp() override
        {
            AWSCoreFixture::SetUp();

            char rootPath[AZ_MAX_PATH_LEN];
            AZ::Utils::GetExecutableDirectory(rootPath, AZ_MAX_PATH_LEN);
            m_localFileIO->SetAlias("@user@", AZ_TRAIT_TEST_ROOT_FOLDER);

            m_localFileIO->ResolvePath("@user@/Registry/", m_resolvedSettingsPath.data(), m_resolvedSettingsPath.size());
            AZ::IO::SystemFile::CreateDir(m_resolvedSettingsPath.data());

            m_localFileIO->ResolvePath("@user@/Registry/editorpreferences.setreg", m_resolvedSettingsPath.data(), m_resolvedSettingsPath.size());

            m_serializeContext = AZStd::make_unique<AZ::SerializeContext>();

            AZ::JsonSystemComponent::Reflect(m_registrationContext.get());

            m_settingsRegistry = AZStd::make_unique<AZ::SettingsRegistryImpl>();

            m_settingsRegistry->SetContext(m_serializeContext.get());
            m_settingsRegistry->SetContext(m_registrationContext.get());

            AZ::SettingsRegistry::Register(m_settingsRegistry.get());

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

            AZ::SettingsRegistry::Unregister(m_settingsRegistry.get());

            m_settingsRegistry.reset();
            m_serializeContext.reset();
            m_registrationContext.reset();

            m_localFileIO->ResolvePath("@user@/Registry/", m_resolvedSettingsPath.data(), m_resolvedSettingsPath.size());
            AZ::IO::SystemFile::DeleteDir(m_resolvedSettingsPath.data());

            delete AZ::IO::FileIOBase::GetInstance();
            AZ::IO::FileIOBase::SetInstance(nullptr);

            AWSCoreFixture::TearDown();
        }
    };

    TEST_F(AttributionManagerTest, MetricsSettings_AttributionDisabled_SkipsSend)
    {
        // GIVEN
        AWSAttributionManagerMock manager;
        manager.Init();
       
        CreateFile(m_resolvedSettingsPath.data(), R"({
            "Amazon": {
                "Preferences": {
                    "EnablePrefabSystem": false,
                    "AWS": {
                        "AWSAttributionEnabled": false,
                        "AWSAttributionDelaySeconds": 30
                    }
                }
            }
        })");

        EXPECT_CALL(manager, SubmitMetric(testing::_)).Times(0);

        // WHEN
        manager.MetricCheck();

        // THEN
        AZ::u64 timeStamp = 0;
        m_settingsRegistry->Get(timeStamp, "/Amazon/Preferences/AWS/AWSAttributionLastTimeStamp");
        ASSERT_TRUE(timeStamp == 0);

        RemoveFile(m_resolvedSettingsPath.data());
    }

    TEST_F(AttributionManagerTest, AttributionEnabled_NoPreviousTimeStamp_SendSuccess)
    {
        // GIVEN
        AWSAttributionManagerMock manager;
        manager.Init();

        CreateFile(m_resolvedSettingsPath.data(), R"({
            "Amazon": {
                "Preferences": {
                    "EnablePrefabSystem": false,
                    "AWS": {
                        "AWSAttributionEnabled": true,
                        "AWSAttributionDelaySeconds": 30,
                    }
                }
            }
        })");

        EXPECT_CALL(manager, SubmitMetric(testing::_)).Times(1);

        // WHEN
        manager.MetricCheck();

        // THEN
        AZ::u64 timeStamp = 0;
        m_settingsRegistry->Get(timeStamp, "/Amazon/Preferences/AWS/AWSAttributionLastTimeStamp");
        ASSERT_TRUE(timeStamp > 0);


        RemoveFile(m_resolvedSettingsPath.data());
    }

    TEST_F(AttributionManagerTest, AttributionEnabled_ValidPreviousTimeStamp_SendSuccess)
    {
        // GIVEN
        AWSAttributionManagerMock manager;
        manager.Init();

        CreateFile(m_resolvedSettingsPath.data(), R"({
            "Amazon": {
                "Preferences": {
                    "EnablePrefabSystem": false,
                    "AWS": {
                        "AWSAttributionEnabled": true,
                        "AWSAttributionDelaySeconds": 30,
                        "AWSAttributionLastTimeStamp": 629400
                    }
                }
            }
        })");

        EXPECT_CALL(manager, SubmitMetric(testing::_)).Times(1);

        // WHEN
        manager.MetricCheck();

        // THEN
        AZ::u64 timeStamp = 0;
        m_settingsRegistry->Get(timeStamp, "/Amazon/Preferences/AWS/AWSAttributionLastTimeStamp");
        ASSERT_TRUE(timeStamp > 0);

        RemoveFile(m_resolvedSettingsPath.data());
    }

    TEST_F(AttributionManagerTest, AttributionEnabled_DelayNotSatisfied_SendFail)
    {
        // GIVEN
        AWSAttributionManagerMock manager;
        manager.Init();


        CreateFile(m_resolvedSettingsPath.data(), R"({
            "Amazon": {
                "Preferences": {
                    "EnablePrefabSystem": false,
                    "AWS": {
                        "AWSAttributionEnabled": true,
                        "AWSAttributionDelaySeconds": 300,
                        "AWSAttributionLastTimeStamp": 0
                    }
                }
            }
        })");

        AZ::u64 delayInSeconds = AZStd::chrono::duration_cast<AZStd::chrono::seconds>(AZStd::chrono::system_clock::now().time_since_epoch()).count();
        ASSERT_TRUE(m_settingsRegistry->Set("/Amazon/Preferences/AWS/AWSAttributionLastTimeStamp", delayInSeconds));

        EXPECT_CALL(manager, SubmitMetric(testing::_)).Times(1);

        // WHEN
        manager.MetricCheck();

        // THEN
        AZ::u64 timeStamp = 0;
        m_settingsRegistry->Get(timeStamp, "/Amazon/Preferences/AWS/AWSAttributionLastTimeStamp");
        ASSERT_TRUE(timeStamp == delayInSeconds);

        RemoveFile(m_resolvedSettingsPath.data());
    }

    TEST_F(AttributionManagerTest, AttributionEnabledNotFound_SendSuccess)
    {
        // GIVEN
        AWSAttributionManagerMock manager;
        manager.Init();

        CreateFile(m_resolvedSettingsPath.data(), R"({
            "Amazon": {
                "Preferences": {
                    "EnablePrefabSystem": false,
                    "AWS": {
                    }
                }
            }
        })");

        EXPECT_CALL(manager, SubmitMetric(testing::_)).Times(1);

        // WHEN
        manager.MetricCheck();

        // THEN
        AZ::u64 timeStamp = 0;
        m_settingsRegistry->Get(timeStamp, "/Amazon/Preferences/AWS/AWSAttributionLastTimeStamp");
        ASSERT_TRUE(timeStamp != 0);

        RemoveFile(m_resolvedSettingsPath.data());
    }
} // namespace AWSCoreUnitTest
