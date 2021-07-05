/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <UserManagement/AWSCognitoUserManagementController.h>
#include <AWSClientAuthGemMock.h>
#include <aws/cognito-idp/CognitoIdentityProviderErrors.h>

class AWSCognitoUserManagementControllerTest
    : public AWSClientAuthUnitTest::AWSClientAuthGemAllocatorFixture
    , public AWSCore::AWSCoreRequestBus::Handler
{
protected:
    void SetUp() override
    {
        AWSClientAuthUnitTest::AWSClientAuthGemAllocatorFixture::SetUp();
        m_mockController = AZStd::make_unique<AWSClientAuth::AWSCognitoUserManagementController>();

        AWSCore::AWSCoreRequestBus::Handler::BusConnect();
    }

    void TearDown() override
    {
        AWSCore::AWSCoreRequestBus::Handler::BusDisconnect();
        m_mockController.reset();
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
    AZStd::unique_ptr<AWSClientAuth::AWSCognitoUserManagementController> m_mockController;
    testing::NiceMock<AWSClientAuthUnitTest::AWSResourceMappingRequestBusMock> m_awsResourceMappingRequestBusMock;
};

TEST_F(AWSCognitoUserManagementControllerTest, Initialize_Success)
{
    EXPECT_CALL(m_awsResourceMappingRequestBusMock, GetResourceNameId(testing::_)).Times(1);
    ASSERT_TRUE(m_mockController->Initialize());
    ASSERT_EQ(m_mockController->GetCognitoAppClientId(), AWSClientAuthUnitTest::TEST_RESOURCE_NAME_ID);
}

TEST_F(AWSCognitoUserManagementControllerTest, EmailSignUp_Success)
{
    EXPECT_CALL(*m_cognitoIdentityProviderClientMock, SignUp(testing::_)).Times(1);
    EXPECT_CALL(m_awsCognitoUserManagementNotificationsBusMock, OnEmailSignUpSuccess(testing::_)).Times(1);
    m_mockController->EmailSignUpAsync(AWSClientAuthUnitTest::TEST_USERNAME, AWSClientAuthUnitTest::TEST_PASSWORD, AWSClientAuthUnitTest::TEST_EMAIL);
}

TEST_F(AWSCognitoUserManagementControllerTest, EmailSignUp_Fail_SignUpError)
{
    Aws::Client::AWSError<Aws::CognitoIdentityProvider::CognitoIdentityProviderErrors> error;
    error.SetExceptionName(AWSClientAuthUnitTest::TEST_EXCEPTION);
    Aws::CognitoIdentityProvider::Model::SignUpOutcome outcome(error);

    EXPECT_CALL(*m_cognitoIdentityProviderClientMock, SignUp(testing::_)).Times(1)
        .WillOnce(testing::Return(outcome));
    EXPECT_CALL(m_awsCognitoUserManagementNotificationsBusMock, OnEmailSignUpSuccess(testing::_)).Times(0);
    EXPECT_CALL(m_awsCognitoUserManagementNotificationsBusMock, OnEmailSignUpFail(testing::_)).Times(1);
    m_mockController->EmailSignUpAsync(AWSClientAuthUnitTest::TEST_USERNAME, AWSClientAuthUnitTest::TEST_PASSWORD, AWSClientAuthUnitTest::TEST_EMAIL);
}

TEST_F(AWSCognitoUserManagementControllerTest, PhoneSignUp_Success)
{
    EXPECT_CALL(*m_cognitoIdentityProviderClientMock, SignUp(testing::_)).Times(1);
    EXPECT_CALL(m_awsCognitoUserManagementNotificationsBusMock, OnPhoneSignUpSuccess(testing::_)).Times(1);
    m_mockController->PhoneSignUpAsync(AWSClientAuthUnitTest::TEST_USERNAME, AWSClientAuthUnitTest::TEST_PASSWORD, AWSClientAuthUnitTest::TEST_PHONE);
}

TEST_F(AWSCognitoUserManagementControllerTest, PhoneSignUp_Fail_SignUpError)
{
    Aws::Client::AWSError<Aws::CognitoIdentityProvider::CognitoIdentityProviderErrors> error;
    error.SetExceptionName(AWSClientAuthUnitTest::TEST_EXCEPTION);
    Aws::CognitoIdentityProvider::Model::SignUpOutcome outcome(error);

    EXPECT_CALL(*m_cognitoIdentityProviderClientMock, SignUp(testing::_)).Times(1)
        .WillOnce(testing::Return(outcome));
    EXPECT_CALL(m_awsCognitoUserManagementNotificationsBusMock, OnPhoneSignUpSuccess(testing::_)).Times(0);
    EXPECT_CALL(m_awsCognitoUserManagementNotificationsBusMock, OnPhoneSignUpFail(testing::_)).Times(1);
    m_mockController->PhoneSignUpAsync(AWSClientAuthUnitTest::TEST_USERNAME, AWSClientAuthUnitTest::TEST_PASSWORD, AWSClientAuthUnitTest::TEST_PHONE);
}

TEST_F(AWSCognitoUserManagementControllerTest, ConfirmSignUp_Success)
{
    EXPECT_CALL(*m_cognitoIdentityProviderClientMock, ConfirmSignUp(testing::_)).Times(1);
    EXPECT_CALL(m_awsCognitoUserManagementNotificationsBusMock, OnConfirmSignUpSuccess()).Times(1);
    m_mockController->ConfirmSignUpAsync(AWSClientAuthUnitTest::TEST_USERNAME, AWSClientAuthUnitTest::TEST_CODE);
}

TEST_F(AWSCognitoUserManagementControllerTest, ConfirmSignUp_Fail_ConfirmSignUpError)
{
    Aws::Client::AWSError<Aws::CognitoIdentityProvider::CognitoIdentityProviderErrors> error;
    error.SetExceptionName(AWSClientAuthUnitTest::TEST_EXCEPTION);
    Aws::CognitoIdentityProvider::Model::ConfirmSignUpOutcome outcome(error);

    EXPECT_CALL(*m_cognitoIdentityProviderClientMock, ConfirmSignUp(testing::_)).Times(1)
        .WillOnce(testing::Return(outcome));
    EXPECT_CALL(m_awsCognitoUserManagementNotificationsBusMock, OnConfirmSignUpSuccess()).Times(0);
    EXPECT_CALL(m_awsCognitoUserManagementNotificationsBusMock, OnConfirmSignUpFail(testing::_)).Times(1);
    m_mockController->ConfirmSignUpAsync(AWSClientAuthUnitTest::TEST_USERNAME, AWSClientAuthUnitTest::TEST_CODE);
}

TEST_F(AWSCognitoUserManagementControllerTest, EnableMFA_Success)
{
    EXPECT_CALL(*m_cognitoIdentityProviderClientMock, SetUserMFAPreference(testing::_)).Times(1);
    EXPECT_CALL(m_awsCognitoUserManagementNotificationsBusMock, OnEnableMFASuccess()).Times(1);
    m_mockController->EnableMFAAsync(AWSClientAuthUnitTest::TEST_TOKEN);
}

TEST_F(AWSCognitoUserManagementControllerTest, EnableMFA_Fail_SetUserMFAPreferenceError)
{
    Aws::Client::AWSError<Aws::CognitoIdentityProvider::CognitoIdentityProviderErrors> error;
    error.SetExceptionName(AWSClientAuthUnitTest::TEST_EXCEPTION);
    Aws::CognitoIdentityProvider::Model::SetUserMFAPreferenceOutcome outcome(error);

    EXPECT_CALL(*m_cognitoIdentityProviderClientMock, SetUserMFAPreference(testing::_)).Times(1)
        .WillOnce(testing::Return(outcome));
    EXPECT_CALL(m_awsCognitoUserManagementNotificationsBusMock, OnEnableMFASuccess()).Times(0);
    EXPECT_CALL(m_awsCognitoUserManagementNotificationsBusMock, OnEnableMFAFail(testing::_)).Times(1);
    m_mockController->EnableMFAAsync(AWSClientAuthUnitTest::TEST_TOKEN);
}

TEST_F(AWSCognitoUserManagementControllerTest, ForgotPassword_Success)
{
    EXPECT_CALL(*m_cognitoIdentityProviderClientMock, ForgotPassword(testing::_)).Times(1);
    EXPECT_CALL(m_awsCognitoUserManagementNotificationsBusMock, OnForgotPasswordSuccess()).Times(1);
    m_mockController->ForgotPasswordAsync(AWSClientAuthUnitTest::TEST_USERNAME);
}

TEST_F(AWSCognitoUserManagementControllerTest, ForgotPassword_Fail_ForgotPasswordError)
{
    Aws::Client::AWSError<Aws::CognitoIdentityProvider::CognitoIdentityProviderErrors> error;
    error.SetExceptionName(AWSClientAuthUnitTest::TEST_EXCEPTION);
    Aws::CognitoIdentityProvider::Model::ForgotPasswordOutcome outcome(error);

    EXPECT_CALL(*m_cognitoIdentityProviderClientMock, ForgotPassword(testing::_)).Times(1)
        .WillOnce(testing::Return(outcome));
    EXPECT_CALL(m_awsCognitoUserManagementNotificationsBusMock, OnForgotPasswordSuccess()).Times(0);
    EXPECT_CALL(m_awsCognitoUserManagementNotificationsBusMock, OnForgotPasswordFail(testing::_)).Times(1);
    m_mockController->ForgotPasswordAsync(AWSClientAuthUnitTest::TEST_USERNAME);
}

TEST_F(AWSCognitoUserManagementControllerTest, ConfirmForgotPassword_Success)
{
    EXPECT_CALL(*m_cognitoIdentityProviderClientMock, ConfirmForgotPassword(testing::_)).Times(1);
    EXPECT_CALL(m_awsCognitoUserManagementNotificationsBusMock, OnConfirmForgotPasswordSuccess()).Times(1);
    m_mockController->ConfirmForgotPasswordAsync(AWSClientAuthUnitTest::TEST_USERNAME, AWSClientAuthUnitTest::TEST_CODE, AWSClientAuthUnitTest::TEST_NEW_PASSWORD);
}

TEST_F(AWSCognitoUserManagementControllerTest, ConfirmForgotPassword_Fail_ConfirmForgotPasswordError)
{
    Aws::Client::AWSError<Aws::CognitoIdentityProvider::CognitoIdentityProviderErrors> error;
    error.SetExceptionName("TestException");
    Aws::CognitoIdentityProvider::Model::ConfirmForgotPasswordOutcome outcome(error);

    EXPECT_CALL(*m_cognitoIdentityProviderClientMock, ConfirmForgotPassword(testing::_)).Times(1)
        .WillOnce(testing::Return(outcome));
    EXPECT_CALL(m_awsCognitoUserManagementNotificationsBusMock, OnConfirmForgotPasswordSuccess()).Times(0);
    EXPECT_CALL(m_awsCognitoUserManagementNotificationsBusMock, OnConfirmForgotPasswordFail(testing::_)).Times(1);
    m_mockController->ConfirmForgotPasswordAsync(AWSClientAuthUnitTest::TEST_USERNAME, AWSClientAuthUnitTest::TEST_CODE, AWSClientAuthUnitTest::TEST_NEW_PASSWORD);
}

TEST_F(AWSCognitoUserManagementControllerTest, Initialize_Fail_GetResourceNameEmpty)
{
    EXPECT_CALL(m_awsResourceMappingRequestBusMock, GetResourceNameId(testing::_)).Times(1).WillOnce(testing::Return(""));
    ASSERT_FALSE(m_mockController->Initialize());
}
