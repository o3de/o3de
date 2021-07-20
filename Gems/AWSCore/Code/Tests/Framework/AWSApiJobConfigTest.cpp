/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Jobs/JobManager.h>
#include <AzCore/Jobs/JobContext.h>
#include <AzTest/AzTest.h>

#include <aws/core/auth/AWSCredentialsProvider.h>

#include <AWSCoreBus.h>
#include <Credential/AWSCredentialBus.h>
#include <Framework/AWSApiJobConfig.h>
#include <TestFramework/AWSCoreFixture.h>

using namespace AWSCore;

class AwsApiJobConfigTest
    : public UnitTest::ScopedAllocatorSetupFixture
    , AWSCredentialRequestBus::Handler
    , AWSCoreRequestBus::Handler
{
public:
    void SetUp() override
    {
        AZ::AllocatorInstance<AZ::ThreadPoolAllocator>::Create();

        m_credentialsHandler = std::make_shared<Aws::Auth::AnonymousAWSCredentialsProvider>();
        AZ::JobManagerDesc jobDesc;
        m_jobManager = AZStd::make_unique<AZ::JobManager>(jobDesc);
        m_jobContext = AZStd::make_unique<AZ::JobContext>(*m_jobManager);
        AWSCredentialRequestBus::Handler::BusConnect();
        AWSCoreRequestBus::Handler::BusConnect();
    }

    void TearDown() override
    {
        AWSCoreRequestBus::Handler::BusDisconnect();
        AWSCredentialRequestBus::Handler::BusDisconnect();
        m_jobContext.reset();
        m_jobManager.reset();
        m_credentialsHandler.reset();

        AZ::AllocatorInstance<AZ::ThreadPoolAllocator>::Destroy();
    }

    // AWSCredentialRequestBus interface implementation
    int GetCredentialHandlerOrder() const override { return -1; }
    std::shared_ptr<Aws::Auth::AWSCredentialsProvider> GetCredentialsProvider() override { return m_credentialsHandler; }

    // AWSCoreRequestBus interface implementation
    AZ::JobContext* GetDefaultJobContext() override { return m_jobContext.get(); }
    AwsApiJobConfig* GetDefaultConfig() override { return nullptr; }

    std::shared_ptr<Aws::Auth::AWSCredentialsProvider> m_credentialsHandler;
    AZStd::unique_ptr<AZ::JobContext> m_jobContext;

private:
    AZStd::unique_ptr<AZ::JobManager> m_jobManager;
};

TEST_F(AwsApiJobConfigTest, ApplySettings_HaveHandlerReturnCredentialsProvider_GetExpectedCredentialsProvider)
{
    AwsApiJobConfig testApiJobConfig;
    testApiJobConfig.ApplySettings();

    EXPECT_TRUE(testApiJobConfig.credentialsProvider);
    EXPECT_TRUE(testApiJobConfig.credentialsProvider == m_credentialsHandler);
}

TEST_F(AwsApiJobConfigTest, GetClientConfiguration_HaveInitializer_GetExpectedConfigurationField)
{
    AwsApiJobConfig testApiJobConfig(nullptr, [](AwsApiJobConfig& config) {
        config.userAgent = "TestUserAgent";
    });
    auto actualConfig = testApiJobConfig.GetClientConfiguration();

    EXPECT_TRUE(actualConfig.userAgent == "TestUserAgent");
}

TEST_F(AwsApiJobConfigTest, GetJobContext_HaveHandlerReturnJobContext_GetExpectedJobContext)
{
    AwsApiJobConfig testApiJobConfig;
    auto actualJobContext = testApiJobConfig.GetJobContext();

    EXPECT_TRUE(actualJobContext);
    EXPECT_TRUE(actualJobContext == m_jobContext.get());
}

TEST_F(AwsApiJobConfigTest, GetConfig_HaveInitializer_GetExpectedConfigurationField)
{
    AwsApiJobConfigHolder<AwsApiJobConfig> testHolder;
    auto actualConfig = testHolder.GetConfig(nullptr, [](AwsApiJobConfig& config) {
        config.userAgent = "TestUserAgent";
    });

    EXPECT_TRUE(actualConfig->userAgent == "TestUserAgent");
}
