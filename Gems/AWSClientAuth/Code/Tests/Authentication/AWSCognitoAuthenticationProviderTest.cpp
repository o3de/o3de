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
#include <AzCore/UnitTest/TestTypes.h>
#include <Authentication/AWSCognitoAuthenticationProvider.h>
#include <AWSClientAuthGemMock.h>
#include <aws/core/utils/Outcome.h>
#include <aws/cognito-idp/CognitoIdentityProviderErrors.h>


namespace AWSClientAuthUnitTest
{
    class AWSCognitoAuthenticationProviderrLocalMock
        : public AWSClientAuth::AWSCognitoAuthenticationProvider
    {
    public:
        using AWSClientAuth::AWSCognitoAuthenticationProvider::m_settings;
    };
}

class AWSCognitoAuthenticationProviderTest
    : public AWSClientAuthUnitTest::AWSClientAuthGemAllocatorFixture
    , public AWSCore::AWSCoreRequestBus::Handler
{
    void SetUp() override
    {
        AWSClientAuthUnitTest::AWSClientAuthGemAllocatorFixture::SetUp();

        AWSClientAuth::AWSCognitoProviderSetting::Reflect(*m_serializeContext);

        AZStd::string path = AZStd::string::format("%s/%s/authenticationProvider.setreg",
            m_testFolder->c_str(), AZ::SettingsRegistryInterface::RegistryFolder);
        CreateTestFile("authenticationProvider.setreg"
            , R"({
                "AWS": 
                {
                    "CognitoIDP":
                    {
                        "AppClientId": "TestCognitoClientId"
                    }
                } 
            })");
        m_settingsRegistry->MergeSettingsFile(path, AZ::SettingsRegistryInterface::Format::JsonMergePatch, {});

        m_cognitoAuthenticationProviderMock.Initialize(m_settingsRegistry);

        AWSCore::AWSCoreRequestBus::Handler::BusConnect();

    }

    void TearDown() override
    {
        AWSCore::AWSCoreRequestBus::Handler::BusDisconnect();
        AWSClientAuthUnitTest::AWSClientAuthGemAllocatorFixture::TearDown();
    }

    // AWSCore::AWSCoreRequestBus overrides
    AZ::JobContext* GetDefaultJobContext() override
    {
        return m_jobContext.get();
    }

    // Returns the default client configuration setting to use as a starting point in AWS requests
    AWSCore::AwsApiJobConfig* GetDefaultConfig() override
    {
        return nullptr;
    }

public:
    AWSClientAuthUnitTest::AWSCognitoAuthenticationProviderrLocalMock m_cognitoAuthenticationProviderMock;

    void AssertAuthenticationTokensPopulated()
    {
        AZ_Assert(
        m_cognitoAuthenticationProviderMock.GetAuthenticationTokens().GetAccessToken() ==
            AWSClientAuthUnitTest::TEST_ACCESS_TOKEN,
        "Access token expected to match");
        AZ_Assert(
            m_cognitoAuthenticationProviderMock.GetAuthenticationTokens().GetOpenIdToken() ==
                AWSClientAuthUnitTest::TEST_ID_TOKEN,
            "Id token expected to match");
        AZ_Assert(
            m_cognitoAuthenticationProviderMock.GetAuthenticationTokens().GetRefreshToken() ==
                AWSClientAuthUnitTest::TEST_REFRESH_TOKEN,
            "Refresh token expected to match");
        AZ_Assert(
            m_cognitoAuthenticationProviderMock.GetAuthenticationTokens().GetTokensExpireTimeSeconds() != 0,
            "Access token expiry expected to be set");
        AZ_Assert(m_cognitoAuthenticationProviderMock.GetAuthenticationTokens().AreTokensValid(), "Tokens expected to be valid");
    }

    void AssertAuthenticationTokensEmpty()
    {
        AZ_Assert(
            m_cognitoAuthenticationProviderMock.GetAuthenticationTokens().GetAccessToken() == "", "Access token expected to be empty");
        AZ_Assert(m_cognitoAuthenticationProviderMock.GetAuthenticationTokens().GetOpenIdToken() == "", "Id token expected to be empty");
        AZ_Assert(
            m_cognitoAuthenticationProviderMock.GetAuthenticationTokens().GetRefreshToken() == "", "Refresh token expected to be empty");
        AZ_Assert(
            m_cognitoAuthenticationProviderMock.GetAuthenticationTokens().GetTokensExpireTimeSeconds() == 0,
            "Access token expiry expected to be 0");
        AZ_Assert(!m_cognitoAuthenticationProviderMock.GetAuthenticationTokens().AreTokensValid(), "Tokens expected to be invalid");
    }

};

TEST_F(AWSCognitoAuthenticationProviderTest, Initialize_Success)
{
    AWSClientAuthUnitTest::AWSCognitoAuthenticationProviderrLocalMock mock;
    ASSERT_TRUE(mock.Initialize(m_settingsRegistry));
    ASSERT_EQ(mock.m_settings->m_appClientId, AWSClientAuthUnitTest::TEST_COGNITO_CLIENTID);
}

TEST_F(AWSCognitoAuthenticationProviderTest, PasswordGrantSingleFactorSignInAsync_Success)
{
    EXPECT_CALL(*m_cognitoIdentityProviderClientMock, InitiateAuth(testing::_)).Times(1);
    EXPECT_CALL(m_authenticationProviderNotificationsBusMock, OnPasswordGrantSingleFactorSignInSuccess(testing::_)).Times(1);
    m_cognitoAuthenticationProviderMock.PasswordGrantSingleFactorSignInAsync(AWSClientAuthUnitTest::TEST_USERNAME, AWSClientAuthUnitTest::TEST_PASSWORD);

    AssertAuthenticationTokensPopulated();
}

TEST_F(AWSCognitoAuthenticationProviderTest, PasswordGrantSingleFactorSignInAsync_Fail_InitiateAuthError)
{
    Aws::Client::AWSError<Aws::CognitoIdentityProvider::CognitoIdentityProviderErrors> error;
    error.SetExceptionName(AWSClientAuthUnitTest::TEST_EXCEPTION);
    Aws::CognitoIdentityProvider::Model::InitiateAuthOutcome outcome(error);

    EXPECT_CALL(*m_cognitoIdentityProviderClientMock, InitiateAuth(testing::_)).Times(1)
        .WillOnce(testing::Return(outcome));
    EXPECT_CALL(m_authenticationProviderNotificationsBusMock, OnPasswordGrantSingleFactorSignInSuccess(testing::_)).Times(0);
    EXPECT_CALL(m_authenticationProviderNotificationsBusMock, OnPasswordGrantSingleFactorSignInFail(testing::_)).Times(1);
    m_cognitoAuthenticationProviderMock.PasswordGrantSingleFactorSignInAsync(AWSClientAuthUnitTest::TEST_USERNAME, AWSClientAuthUnitTest::TEST_PASSWORD);

    AssertAuthenticationTokensEmpty();
}

TEST_F(AWSCognitoAuthenticationProviderTest, PasswordGrantSingleFactorSignInAsync_Fail_IncorrectChallengeTypeError)
{
    Aws::CognitoIdentityProvider::Model::InitiateAuthResult result;
    result.SetChallengeName(Aws::CognitoIdentityProvider::Model::ChallengeNameType::CUSTOM_CHALLENGE);
    Aws::CognitoIdentityProvider::Model::InitiateAuthOutcome outcome(result);

    EXPECT_CALL(*m_cognitoIdentityProviderClientMock, InitiateAuth(testing::_)).Times(1)
        .WillOnce(testing::Return(outcome));
    EXPECT_CALL(m_authenticationProviderNotificationsBusMock, OnPasswordGrantSingleFactorSignInSuccess(testing::_)).Times(0);
    EXPECT_CALL(m_authenticationProviderNotificationsBusMock, OnPasswordGrantSingleFactorSignInFail(testing::_)).Times(1);
    m_cognitoAuthenticationProviderMock.PasswordGrantSingleFactorSignInAsync(AWSClientAuthUnitTest::TEST_USERNAME, AWSClientAuthUnitTest::TEST_PASSWORD);
}

TEST_F(AWSCognitoAuthenticationProviderTest, PasswordGrantMultiFactorSignInAsync_Success)
{
    Aws::CognitoIdentityProvider::Model::InitiateAuthResult result;
    result.SetChallengeName(Aws::CognitoIdentityProvider::Model::ChallengeNameType::SMS_MFA);
    result.SetSession(AWSClientAuthUnitTest::TEST_SESSION);
    Aws::CognitoIdentityProvider::Model::InitiateAuthOutcome outcome(result);

    EXPECT_CALL(*m_cognitoIdentityProviderClientMock, InitiateAuth(testing::_)).Times(1)
        .WillOnce(testing::Return(outcome));
    EXPECT_CALL(m_authenticationProviderNotificationsBusMock, OnPasswordGrantMultiFactorSignInSuccess()).Times(1);
    m_cognitoAuthenticationProviderMock.PasswordGrantMultiFactorSignInAsync(AWSClientAuthUnitTest::TEST_USERNAME, AWSClientAuthUnitTest::TEST_PASSWORD);
}

TEST_F(AWSCognitoAuthenticationProviderTest, PasswordGrantMultiFactorSignInAsync_Fail_InitiateAuthError)
{
    Aws::Client::AWSError<Aws::CognitoIdentityProvider::CognitoIdentityProviderErrors> error;
    error.SetExceptionName(AWSClientAuthUnitTest::TEST_EXCEPTION);
    Aws::CognitoIdentityProvider::Model::InitiateAuthOutcome outcome(error);


    EXPECT_CALL(*m_cognitoIdentityProviderClientMock, InitiateAuth(testing::_)).Times(1)
        .WillOnce(testing::Return(outcome));
    EXPECT_CALL(m_authenticationProviderNotificationsBusMock, OnPasswordGrantMultiFactorSignInSuccess()).Times(0);
    EXPECT_CALL(m_authenticationProviderNotificationsBusMock, OnPasswordGrantMultiFactorSignInFail(testing::_)).Times(1);
    m_cognitoAuthenticationProviderMock.PasswordGrantMultiFactorSignInAsync(AWSClientAuthUnitTest::TEST_USERNAME, AWSClientAuthUnitTest::TEST_PASSWORD);
}

TEST_F(AWSCognitoAuthenticationProviderTest, PasswordGrantMultiFactorSignInAsync_Fail_IncorrectChallengeTypeError)
{
    Aws::CognitoIdentityProvider::Model::InitiateAuthResult result;
    result.SetChallengeName(Aws::CognitoIdentityProvider::Model::ChallengeNameType::CUSTOM_CHALLENGE);
    Aws::CognitoIdentityProvider::Model::InitiateAuthOutcome outcome(result);

    EXPECT_CALL(*m_cognitoIdentityProviderClientMock, InitiateAuth(testing::_)).Times(1)
        .WillOnce(testing::Return(outcome));
    EXPECT_CALL(m_authenticationProviderNotificationsBusMock, OnPasswordGrantMultiFactorSignInSuccess()).Times(0);
    EXPECT_CALL(m_authenticationProviderNotificationsBusMock, OnPasswordGrantMultiFactorSignInFail(testing::_)).Times(1);
    m_cognitoAuthenticationProviderMock.PasswordGrantMultiFactorSignInAsync(AWSClientAuthUnitTest::TEST_USERNAME, AWSClientAuthUnitTest::TEST_PASSWORD);
}

TEST_F(AWSCognitoAuthenticationProviderTest, PasswordGrantMultiFactorConfirmSignInAsync_Success)
{
    EXPECT_CALL(*m_cognitoIdentityProviderClientMock, RespondToAuthChallenge(testing::_)).Times(1);
    EXPECT_CALL(m_authenticationProviderNotificationsBusMock, OnPasswordGrantMultiFactorConfirmSignInSuccess(testing::_)).Times(1);
    m_cognitoAuthenticationProviderMock.PasswordGrantMultiFactorConfirmSignInAsync(AWSClientAuthUnitTest::TEST_USERNAME, AWSClientAuthUnitTest::TEST_PASSWORD);

    AssertAuthenticationTokensPopulated();
}

TEST_F(AWSCognitoAuthenticationProviderTest, PasswordGrantMultiFactorConfirmSignInAsync_Fail_RespondToAuthChallengeError)
{
    Aws::Client::AWSError<Aws::CognitoIdentityProvider::CognitoIdentityProviderErrors> error;
    error.SetExceptionName(AWSClientAuthUnitTest::TEST_EXCEPTION);
    Aws::CognitoIdentityProvider::Model::RespondToAuthChallengeOutcome outcome(error);

    EXPECT_CALL(*m_cognitoIdentityProviderClientMock, RespondToAuthChallenge(testing::_)).Times(1)
        .WillOnce(testing::Return(outcome));
    EXPECT_CALL(m_authenticationProviderNotificationsBusMock, OnPasswordGrantMultiFactorConfirmSignInSuccess(testing::_)).Times(0);
    EXPECT_CALL(m_authenticationProviderNotificationsBusMock, OnPasswordGrantMultiFactorConfirmSignInFail(testing::_)).Times(1);
    m_cognitoAuthenticationProviderMock.PasswordGrantMultiFactorConfirmSignInAsync(AWSClientAuthUnitTest::TEST_USERNAME, AWSClientAuthUnitTest::TEST_PASSWORD);

    AssertAuthenticationTokensEmpty();
}

TEST_F(AWSCognitoAuthenticationProviderTest, RefreshTokensAsync_Success)
{
    EXPECT_CALL(*m_cognitoIdentityProviderClientMock, InitiateAuth(testing::_)).Times(1);
    EXPECT_CALL(m_authenticationProviderNotificationsBusMock, OnRefreshTokensSuccess(testing::_)).Times(1);
    EXPECT_CALL(m_authenticationProviderNotificationsBusMock, OnRefreshTokensFail(testing::_)).Times(0);
    m_cognitoAuthenticationProviderMock.RefreshTokensAsync();

    AssertAuthenticationTokensPopulated();
}

TEST_F(AWSCognitoAuthenticationProviderTest, RefreshTokensAsync_Fail_InitiateAuthError)
{
    Aws::Client::AWSError<Aws::CognitoIdentityProvider::CognitoIdentityProviderErrors> error;
    error.SetExceptionName(AWSClientAuthUnitTest::TEST_EXCEPTION);
    Aws::CognitoIdentityProvider::Model::InitiateAuthOutcome outcome(error);

    EXPECT_CALL(*m_cognitoIdentityProviderClientMock, InitiateAuth(testing::_)).Times(1)
        .WillOnce(testing::Return(outcome));
    EXPECT_CALL(m_authenticationProviderNotificationsBusMock, OnRefreshTokensSuccess(testing::_)).Times(0);
    EXPECT_CALL(m_authenticationProviderNotificationsBusMock, OnRefreshTokensFail(testing::_)).Times(1);
    m_cognitoAuthenticationProviderMock.RefreshTokensAsync();

    AssertAuthenticationTokensEmpty();
}

TEST_F(AWSCognitoAuthenticationProviderTest, RefreshTokensAsync_Fail_IncorrectChallengeType)
{
    Aws::CognitoIdentityProvider::Model::InitiateAuthResult result;
    result.SetChallengeName(Aws::CognitoIdentityProvider::Model::ChallengeNameType::CUSTOM_CHALLENGE);
    Aws::CognitoIdentityProvider::Model::InitiateAuthOutcome outcome(result);

    EXPECT_CALL(*m_cognitoIdentityProviderClientMock, InitiateAuth(testing::_)).Times(1)
        .WillOnce(testing::Return(outcome));
    EXPECT_CALL(m_authenticationProviderNotificationsBusMock, OnRefreshTokensSuccess(testing::_)).Times(0);
    EXPECT_CALL(m_authenticationProviderNotificationsBusMock, OnRefreshTokensFail(testing::_)).Times(1);
    m_cognitoAuthenticationProviderMock.RefreshTokensAsync();

    AssertAuthenticationTokensEmpty();
}

TEST_F(AWSCognitoAuthenticationProviderTest, SignOut_Success)
{
    EXPECT_CALL(*m_cognitoIdentityProviderClientMock, InitiateAuth(testing::_)).Times(1);
    EXPECT_CALL(m_authenticationProviderNotificationsBusMock, OnPasswordGrantSingleFactorSignInSuccess(testing::_)).Times(1);
    m_cognitoAuthenticationProviderMock.PasswordGrantSingleFactorSignInAsync(AWSClientAuthUnitTest::TEST_USERNAME, AWSClientAuthUnitTest::TEST_PASSWORD);

    AssertAuthenticationTokensPopulated();

    m_cognitoAuthenticationProviderMock.SignOut();

    AssertAuthenticationTokensEmpty();
}

TEST_F(AWSCognitoAuthenticationProviderTest, Initialize_Fail_EmptyRegistry)
{
    AWSClientAuthUnitTest::AWSCognitoAuthenticationProviderrLocalMock mock;
    AZStd::shared_ptr<AZ::SettingsRegistryImpl> registry = AZStd::make_shared<AZ::SettingsRegistryImpl>();
    registry->SetContext(m_serializeContext.get());
    ASSERT_FALSE(mock.Initialize(registry));
    ASSERT_EQ(mock.m_settings->m_appClientId, "");
    registry.reset();

    // Restore
    mock.Initialize(m_settingsRegistry);
}
