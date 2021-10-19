/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/UnitTest/TestTypes.h>
#include <AzTest/AzTest.h>

#include <AWSCoreInternalBus.h>
#include <Credential/AWSDefaultCredentialHandler.h>

using namespace AWSCore;

static constexpr char AWSDEFAULTCREDENTIALHANDLERTEST_ALLOC_TAG[] = "AWSDefaultCredentialHandlerTest";
static constexpr const char* AWS_ACCESS_KEY = "AWSACCESSKEY";
static constexpr const char* AWS_SECRET_KEY = "AWSSECRETKEY";

class EnvironmentAWSCredentialsProviderMock : public Aws::Auth::EnvironmentAWSCredentialsProvider
{
public:
    MOCK_METHOD0(GetAWSCredentials, Aws::Auth::AWSCredentials());
};

class ProfileConfigFileAWSCredentialsProviderMock : public Aws::Auth::ProfileConfigFileAWSCredentialsProvider
{
public:
    MOCK_METHOD0(GetAWSCredentials, Aws::Auth::AWSCredentials());
};

class AWSDefaultCredentialHandlerMock
    : public AWSDefaultCredentialHandler
{
public:
    void SetupMocks(
        std::shared_ptr<EnvironmentAWSCredentialsProviderMock> environmentCredentialsProviderMock,
        std::shared_ptr<ProfileConfigFileAWSCredentialsProviderMock> profileCredentialsProviderMock)
    {
        SetEnvironmentCredentialsProvider(environmentCredentialsProviderMock);
        SetProfileCredentialsProvider(profileCredentialsProviderMock);
    }
};

class AWSDefaultCredentialHandlerTest
    : public UnitTest::ScopedAllocatorSetupFixture
    , public AWSCoreInternalRequestBus::Handler
{
public:
    AWSDefaultCredentialHandlerTest() = default;
    ~AWSDefaultCredentialHandlerTest() override = default;

    void SetUp() override
    {
        AWSCoreInternalRequestBus::Handler::BusConnect();
        m_environmentCredentialsProviderMock = Aws::MakeShared<EnvironmentAWSCredentialsProviderMock>(AWSDEFAULTCREDENTIALHANDLERTEST_ALLOC_TAG);
        m_profileCredentialsProviderMock = Aws::MakeShared<ProfileConfigFileAWSCredentialsProviderMock>(AWSDEFAULTCREDENTIALHANDLERTEST_ALLOC_TAG);
        m_credentialHandler = AZStd::make_unique<AWSDefaultCredentialHandlerMock>();
        m_credentialHandler->ActivateHandler();
        m_credentialHandler->SetupMocks(m_environmentCredentialsProviderMock, m_profileCredentialsProviderMock);
    }

    void TearDown() override
    {
        m_credentialHandler->DeactivateHandler();
        m_credentialHandler.reset();
        m_profileCredentialsProviderMock.reset();
        m_environmentCredentialsProviderMock.reset();
        AWSCoreInternalRequestBus::Handler::BusDisconnect();
    }

    // AWSCoreInternalRequestBus interface implementation
    AZStd::string GetProfileName() const override { return m_profileName; }
    AZStd::string GetResourceMappingConfigFilePath() const override { return ""; }
    void ReloadConfiguration() override {}

    std::shared_ptr<EnvironmentAWSCredentialsProviderMock> m_environmentCredentialsProviderMock;
    std::shared_ptr<ProfileConfigFileAWSCredentialsProviderMock> m_profileCredentialsProviderMock;
    AZStd::unique_ptr<AWSDefaultCredentialHandlerMock> m_credentialHandler;
    AZStd::string m_profileName;
};

TEST_F(AWSDefaultCredentialHandlerTest, GetCredentialsProvider_EnvironmentCredentialProviderReturnsNonEmptyCredentials_GetExpectedCredentialProvider)
{
    Aws::Auth::AWSCredentials nonEmptyCredential(AWS_ACCESS_KEY, AWS_SECRET_KEY);
    EXPECT_CALL(*m_environmentCredentialsProviderMock, GetAWSCredentials()).Times(1).WillOnce(::testing::Return(nonEmptyCredential));
    EXPECT_CALL(*m_profileCredentialsProviderMock, GetAWSCredentials()).Times(0);
    auto credentialProvider = m_credentialHandler->GetCredentialsProvider();
    EXPECT_TRUE(credentialProvider == m_environmentCredentialsProviderMock);
}

TEST_F(AWSDefaultCredentialHandlerTest, GetCredentialsProvider_ProfileCredentialProviderReturnsNonEmptyCredentials_GetExpectedCredentialProvider)
{
    Aws::Auth::AWSCredentials emptyCredential;
    Aws::Auth::AWSCredentials nonEmptyCredential(AWS_ACCESS_KEY, AWS_SECRET_KEY);
    EXPECT_CALL(*m_environmentCredentialsProviderMock, GetAWSCredentials()).Times(1).WillOnce(::testing::Return(emptyCredential));
    EXPECT_CALL(*m_profileCredentialsProviderMock, GetAWSCredentials()).Times(1).WillOnce(::testing::Return(nonEmptyCredential));
    auto credentialProvider = m_credentialHandler->GetCredentialsProvider();
    EXPECT_TRUE(credentialProvider == m_profileCredentialsProviderMock);
}

TEST_F(AWSDefaultCredentialHandlerTest, GetCredentialsProvider_ProfileNameHasBeenChanged_GetDifferentCredentialProvider)
{
    Aws::Auth::AWSCredentials emptyCredential;
    Aws::Auth::AWSCredentials nonEmptyCredential(AWS_ACCESS_KEY, AWS_SECRET_KEY);
    EXPECT_CALL(*m_environmentCredentialsProviderMock, GetAWSCredentials()).Times(1).WillOnce(::testing::Return(emptyCredential));
    EXPECT_CALL(*m_profileCredentialsProviderMock, GetAWSCredentials()).Times(0);
    m_profileName = "dummyProfile";
    auto credentialProvider = m_credentialHandler->GetCredentialsProvider();
    EXPECT_TRUE(credentialProvider != m_profileCredentialsProviderMock);
}

TEST_F(AWSDefaultCredentialHandlerTest, GetCredentialsProvider_NoCredentialFoundInChain_GetNullPointer)
{
    Aws::Auth::AWSCredentials emptyCredential;
    EXPECT_CALL(*m_environmentCredentialsProviderMock, GetAWSCredentials()).Times(1).WillOnce(::testing::Return(emptyCredential));
    EXPECT_CALL(*m_profileCredentialsProviderMock, GetAWSCredentials()).Times(1).WillOnce(::testing::Return(emptyCredential));
    auto credentialProvider = m_credentialHandler->GetCredentialsProvider();
    EXPECT_FALSE(credentialProvider);
}

TEST_F(AWSDefaultCredentialHandlerTest, GetCredentialHandlerOrder_Call_AlwaysGetExpectedValue)
{
    auto actualOrder = m_credentialHandler->GetCredentialHandlerOrder();
    EXPECT_EQ(actualOrder, CredentialHandlerOrder::DEFAULT_CREDENTIAL_HANDLER);
}
