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

#include <AzTest/AzTest.h>

#include <AWSNativeSDKInit/AWSNativeSDKInit.h>
#include <aws/core/auth/AWSCredentialsProvider.h>
#include <aws/s3/S3Client.h>

#include <Credential/AWSCredentialBus.h>
#include <Framework/AWSApiClientJobConfig.h>
#include <TestFramework/AWSCoreFixture.h>

using namespace AWSCore;

class AWSApiClientJobConfigTest
    : public UnitTest::ScopedAllocatorSetupFixture
    , public AWSCredentialRequestBus::Handler
{
public:
    void SetUp() override
    {
        AWSNativeSDKInit::InitializationManager::InitAwsApi();
        m_credentialHandlerCounter = 0;
    }

    void TearDown() override
    {
        AWSNativeSDKInit::InitializationManager::Shutdown();
    }

    // AWSCredentialRequestBus interface implementation
    int GetCredentialHandlerOrder() const override { return -1; }
    std::shared_ptr<Aws::Auth::AWSCredentialsProvider> GetCredentialsProvider() override
    {
        m_credentialHandlerCounter++;
        return m_credentialsHandler;
    }

    std::shared_ptr<Aws::Auth::AWSCredentialsProvider> m_credentialsHandler;
    int m_credentialHandlerCounter;
};

TEST_F(AWSApiClientJobConfigTest, GetClient_HaveHandlerReturnCredentialsProvider_CreateClientWithExpectedCredential)
{
    AWSCredentialRequestBus::Handler::BusConnect();
    m_credentialsHandler = std::make_shared<Aws::Auth::AnonymousAWSCredentialsProvider>();
    AwsApiClientJobConfig<Aws::S3::S3Client> testClientJobConfig;
    auto actualClient = testClientJobConfig.GetClient();
    EXPECT_TRUE(m_credentialHandlerCounter == 1);
    EXPECT_TRUE(actualClient);
    AWSCredentialRequestBus::Handler::BusDisconnect();
}

TEST_F(AWSApiClientJobConfigTest, GetClient_NoHandlerReturnCredentialsProvider_CreateClientWithDefaultCredential)
{
    AwsApiClientJobConfig<Aws::S3::S3Client> testClientJobConfig;
    auto actualClient = testClientJobConfig.GetClient();
    EXPECT_TRUE(m_credentialHandlerCounter == 0);
    EXPECT_TRUE(actualClient);
}
