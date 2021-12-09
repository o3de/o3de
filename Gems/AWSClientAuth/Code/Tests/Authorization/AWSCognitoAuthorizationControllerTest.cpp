/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AWSClientAuthGemMock.h>
#include <Authorization/AWSCognitoAuthorizationController.h>
#include <Credential/AWSCredentialBus.h>
#include <aws/cognito-identity/CognitoIdentityErrors.h>

namespace AWSClientAuthUnitTest
{
    class AWSCognitoAuthorizationControllerTestLocalMock
        : public AWSClientAuth::AWSCognitoAuthorizationController

    {
    public:
        using AWSClientAuth::AWSCognitoAuthorizationController::m_persistentCognitoIdentityProvider;
        using AWSClientAuth::AWSCognitoAuthorizationController::m_persistentAnonymousCognitoIdentityProvider;
        using AWSClientAuth::AWSCognitoAuthorizationController::m_cognitoCachingCredentialsProvider;
        using AWSClientAuth::AWSCognitoAuthorizationController::m_cognitoCachingAnonymousCredentialsProvider;
        using AWSClientAuth::AWSCognitoAuthorizationController::m_cognitoIdentityPoolId;
        using AWSClientAuth::AWSCognitoAuthorizationController::m_formattedCognitoUserPoolId;
        using AWSClientAuth::AWSCognitoAuthorizationController::m_awsAccountId;
    };
}

class AWSCognitoAuthorizationControllerTest
    : public AWSClientAuthUnitTest::AWSClientAuthGemAllocatorFixture
{
protected:
    void SetUp() override
    {
        AWSClientAuthUnitTest::AWSClientAuthGemAllocatorFixture::SetUp();
        m_mockController = AZStd::make_unique<AWSClientAuthUnitTest::AWSCognitoAuthorizationControllerTestLocalMock>();
    }

    void TearDown() override
    {
        m_mockController.reset();
        AWSClientAuthUnitTest::AWSClientAuthGemAllocatorFixture::TearDown();
    }

public:
    AZStd::unique_ptr<AWSClientAuthUnitTest::AWSCognitoAuthorizationControllerTestLocalMock> m_mockController;
    testing::NiceMock<AWSClientAuthUnitTest::AWSResourceMappingRequestBusMock> m_awsResourceMappingRequestBusMock;
};

TEST_F(AWSCognitoAuthorizationControllerTest, Initialize_Success)
{
    EXPECT_CALL(m_awsResourceMappingRequestBusMock, GetResourceNameId(testing::_)).Times(2);
    EXPECT_CALL(m_awsResourceMappingRequestBusMock, GetDefaultAccountId()).Times(1);
    EXPECT_CALL(m_awsResourceMappingRequestBusMock, GetDefaultRegion()).Times(1);
    ASSERT_TRUE(m_mockController->Initialize());
    ASSERT_TRUE(m_mockController->m_formattedCognitoUserPoolId.find(AWSClientAuthUnitTest::TEST_RESOURCE_NAME_ID) != AZStd::string::npos);
    ASSERT_TRUE(m_mockController->m_awsAccountId == AWSClientAuthUnitTest::TEST_ACCOUNT_ID);
    ASSERT_TRUE(m_mockController->m_cognitoIdentityPoolId == AWSClientAuthUnitTest::TEST_RESOURCE_NAME_ID);
}

TEST_F(AWSCognitoAuthorizationControllerTest, Initialize_Success_GetAWSAccountEmpty)
{
    EXPECT_CALL(m_awsResourceMappingRequestBusMock, GetResourceNameId(testing::_)).Times(2);
    EXPECT_CALL(m_awsResourceMappingRequestBusMock, GetDefaultAccountId()).Times(1).WillOnce(testing::Return(""));
    EXPECT_CALL(m_awsResourceMappingRequestBusMock, GetDefaultRegion()).Times(1);
    ASSERT_TRUE(m_mockController->Initialize());
}

TEST_F(AWSCognitoAuthorizationControllerTest, RequestAWSCredentials_WithLogins_Success)
{
    AWSClientAuth::AuthenticationTokens tokens(
        AWSClientAuthUnitTest::TEST_TOKEN, AWSClientAuthUnitTest::TEST_TOKEN, AWSClientAuthUnitTest::TEST_TOKEN,
        AWSClientAuth::ProviderNameEnum::AWSCognitoIDP, 60);

    EXPECT_CALL(m_authenticationProviderNotificationsBusMock, OnPasswordGrantSingleFactorSignInSuccess(testing::_)).Times(1);
    AWSClientAuth::AuthenticationProviderNotificationBus::Broadcast(
        &AWSClientAuth::AuthenticationProviderNotifications::OnPasswordGrantSingleFactorSignInSuccess, tokens);

    AWSClientAuth::AuthenticationTokens tokens1(
        AWSClientAuthUnitTest::TEST_TOKEN, AWSClientAuthUnitTest::TEST_TOKEN, AWSClientAuthUnitTest::TEST_TOKEN,
        AWSClientAuth::ProviderNameEnum::Google, 60);
    EXPECT_CALL(m_authenticationProviderNotificationsBusMock, OnDeviceCodeGrantConfirmSignInSuccess(testing::_)).Times(1);
    AWSClientAuth::AuthenticationProviderNotificationBus::Broadcast(
        &AWSClientAuth::AuthenticationProviderNotifications::OnDeviceCodeGrantConfirmSignInSuccess, tokens1);

    EXPECT_CALL(*m_cognitoIdentityClientMock, GetId(testing::_)).Times(1);
    EXPECT_CALL(*m_cognitoIdentityClientMock, GetCredentialsForIdentity(testing::_)).Times(1);
    EXPECT_CALL(m_awsCognitoAuthorizationNotificationsBusMock, OnRequestAWSCredentialsSuccess(testing::_)).Times(1);
    m_mockController->RequestAWSCredentialsAsync();

    ASSERT_TRUE(m_mockController->GetIdentityId() == AWSClientAuthUnitTest::TEST_IDENTITY_ID);
    auto creds = m_mockController->GetCognitoCredentialsProvider()->GetAWSCredentials();
    ASSERT_TRUE(creds.GetAWSAccessKeyId() == AWSClientAuthUnitTest::TEST_ACCESS_KEY_ID);
    ASSERT_TRUE(creds.GetAWSSecretKey() == AWSClientAuthUnitTest::TEST_SECRET_KEY_ID);
}

TEST_F(AWSCognitoAuthorizationControllerTest, RequestAWSCredentials_WithoutLoginsAnonymous_Success)
{
    EXPECT_CALL(*m_cognitoIdentityClientMock, GetId(testing::_)).Times(1);
    EXPECT_CALL(m_awsCognitoAuthorizationNotificationsBusMock, OnRequestAWSCredentialsSuccess(testing::_)).Times(1);
    EXPECT_CALL(*m_cognitoIdentityClientMock, GetCredentialsForIdentity(testing::_)).Times(1);
    m_mockController->RequestAWSCredentialsAsync();

    ASSERT_TRUE(m_mockController->GetIdentityId() == AWSClientAuthUnitTest::TEST_IDENTITY_ID);
    auto creds = m_mockController->GetAnonymousCognitoCredentialsProvider()->GetAWSCredentials();
    ASSERT_TRUE(creds.GetAWSAccessKeyId() == AWSClientAuthUnitTest::TEST_ACCESS_KEY_ID);
    ASSERT_TRUE(creds.GetAWSSecretKey() == AWSClientAuthUnitTest::TEST_SECRET_KEY_ID);
}

TEST_F(AWSCognitoAuthorizationControllerTest, MultipleCalls_UsesCacheCredentials_Success)
{
    EXPECT_CALL(*m_cognitoIdentityClientMock, GetId(testing::_)).Times(1);
    EXPECT_CALL(*m_cognitoIdentityClientMock, GetCredentialsForIdentity(testing::_)).Times(1);
    EXPECT_CALL(m_awsCognitoAuthorizationNotificationsBusMock, OnRequestAWSCredentialsSuccess(testing::_)).Times(1);
    m_mockController->RequestAWSCredentialsAsync();

    ASSERT_TRUE(m_mockController->GetIdentityId() == AWSClientAuthUnitTest::TEST_IDENTITY_ID);
    auto creds = m_mockController->GetAnonymousCognitoCredentialsProvider()->GetAWSCredentials();
    ASSERT_TRUE(creds.GetAWSAccessKeyId() == AWSClientAuthUnitTest::TEST_ACCESS_KEY_ID);
    ASSERT_TRUE(creds.GetAWSSecretKey() == AWSClientAuthUnitTest::TEST_SECRET_KEY_ID);

    EXPECT_CALL(*m_cognitoIdentityClientMock, GetId(testing::_)).Times(0);
    EXPECT_CALL(*m_cognitoIdentityClientMock, GetCredentialsForIdentity(testing::_)).Times(0);
    EXPECT_CALL(m_awsCognitoAuthorizationNotificationsBusMock, OnRequestAWSCredentialsSuccess(testing::_)).Times(1);
    m_mockController->RequestAWSCredentialsAsync();
}

TEST_F(AWSCognitoAuthorizationControllerTest, RequestAWSCredentials_Fail_GetIdError) // fail
{
    AWSClientAuth::AuthenticationTokens cognitoTokens(
        AWSClientAuthUnitTest::TEST_TOKEN, AWSClientAuthUnitTest::TEST_TOKEN, AWSClientAuthUnitTest::TEST_TOKEN,
        AWSClientAuth::ProviderNameEnum::AWSCognitoIDP, 60);
    
    EXPECT_CALL(m_authenticationProviderNotificationsBusMock, OnPasswordGrantSingleFactorSignInSuccess(testing::_)).Times(1);
    AWSClientAuth::AuthenticationProviderNotificationBus::Broadcast(
        &AWSClientAuth::AuthenticationProviderNotifications::OnPasswordGrantSingleFactorSignInSuccess, cognitoTokens);

    ASSERT_TRUE(m_mockController->m_persistentCognitoIdentityProvider->GetLogins().size() == 1);

    Aws::Client::AWSError<Aws::CognitoIdentity::CognitoIdentityErrors> error;
    error.SetExceptionName(AWSClientAuthUnitTest::TEST_EXCEPTION);
    Aws::CognitoIdentity::Model::GetIdOutcome outcome(error);
    EXPECT_CALL(*m_cognitoIdentityClientMock, GetId(testing::_)).Times(1).WillOnce(testing::Return(outcome));
    EXPECT_CALL(*m_cognitoIdentityClientMock, GetCredentialsForIdentity(testing::_)).Times(0);
    EXPECT_CALL(m_awsCognitoAuthorizationNotificationsBusMock, OnRequestAWSCredentialsSuccess(testing::_)).Times(0);
    EXPECT_CALL(m_awsCognitoAuthorizationNotificationsBusMock, OnRequestAWSCredentialsFail(testing::_)).Times(1);
    AZ_TEST_START_TRACE_SUPPRESSION;
    m_mockController->RequestAWSCredentialsAsync();
    AZ_TEST_STOP_TRACE_SUPPRESSION_NO_COUNT;
}

TEST_F(AWSCognitoAuthorizationControllerTest, RequestAWSCredentials_Fail_GetCredentialsForIdentityError)
{
    AWSClientAuth::AuthenticationTokens cognitoTokens(
        AWSClientAuthUnitTest::TEST_TOKEN, AWSClientAuthUnitTest::TEST_TOKEN, AWSClientAuthUnitTest::TEST_TOKEN,
        AWSClientAuth::ProviderNameEnum::AWSCognitoIDP, 60);
    EXPECT_CALL(m_authenticationProviderNotificationsBusMock, OnPasswordGrantSingleFactorSignInSuccess(testing::_)).Times(1);
    AWSClientAuth::AuthenticationProviderNotificationBus::Broadcast(
        &AWSClientAuth::AuthenticationProviderNotifications::OnPasswordGrantSingleFactorSignInSuccess, cognitoTokens);

    AWSClientAuth::AuthenticationTokens googleTokens(
        AWSClientAuthUnitTest::TEST_TOKEN, AWSClientAuthUnitTest::TEST_TOKEN, AWSClientAuthUnitTest::TEST_TOKEN,
        AWSClientAuth::ProviderNameEnum::Google, 60);

    EXPECT_CALL(m_authenticationProviderNotificationsBusMock, OnDeviceCodeGrantConfirmSignInSuccess(testing::_)).Times(1);
    AWSClientAuth::AuthenticationProviderNotificationBus::Broadcast(
        &AWSClientAuth::AuthenticationProviderNotifications::OnDeviceCodeGrantConfirmSignInSuccess, googleTokens);

    ASSERT_TRUE(m_mockController->m_persistentCognitoIdentityProvider->GetLogins().size() == 2);
    EXPECT_CALL(m_authenticationProviderNotificationsBusMock, OnDeviceCodeGrantConfirmSignInSuccess(testing::_)).Times(1);
    AWSClientAuth::AuthenticationProviderNotificationBus::Broadcast(
        &AWSClientAuth::AuthenticationProviderNotifications::OnDeviceCodeGrantConfirmSignInSuccess, googleTokens);

    ASSERT_TRUE(m_mockController->m_persistentCognitoIdentityProvider->GetLogins().size() == 2);

    Aws::Client::AWSError<Aws::CognitoIdentity::CognitoIdentityErrors> error;
    error.SetExceptionName(AWSClientAuthUnitTest::TEST_EXCEPTION);
    Aws::CognitoIdentity::Model::GetCredentialsForIdentityOutcome outcome(error);
    EXPECT_CALL(*m_cognitoIdentityClientMock, GetId(testing::_)).Times(1);
    EXPECT_CALL(*m_cognitoIdentityClientMock, GetCredentialsForIdentity(testing::_)).Times(1).WillOnce(testing::Return(outcome));
    EXPECT_CALL(m_awsCognitoAuthorizationNotificationsBusMock, OnRequestAWSCredentialsSuccess(testing::_)).Times(0);
    EXPECT_CALL(m_awsCognitoAuthorizationNotificationsBusMock, OnRequestAWSCredentialsFail(testing::_)).Times(1);
    AZ_TEST_START_TRACE_SUPPRESSION;
    m_mockController->RequestAWSCredentialsAsync();
    AZ_TEST_STOP_TRACE_SUPPRESSION_NO_COUNT;
}

TEST_F(AWSCognitoAuthorizationControllerTest, AddRemoveLogins_Succuess)
{
    AWSClientAuth::AuthenticationTokens cognitoTokens(
        AWSClientAuthUnitTest::TEST_TOKEN, AWSClientAuthUnitTest::TEST_TOKEN, AWSClientAuthUnitTest::TEST_TOKEN,
        AWSClientAuth::ProviderNameEnum::AWSCognitoIDP, 60);

    EXPECT_CALL(m_authenticationProviderNotificationsBusMock, OnPasswordGrantSingleFactorSignInSuccess(testing::_)).Times(1);
    AWSClientAuth::AuthenticationProviderNotificationBus::Broadcast(
        &AWSClientAuth::AuthenticationProviderNotifications::OnPasswordGrantSingleFactorSignInSuccess, cognitoTokens);


    ASSERT_TRUE(m_mockController->m_persistentCognitoIdentityProvider->GetLogins().size() == 1);

    EXPECT_CALL(m_authenticationProviderNotificationsBusMock, OnPasswordGrantSingleFactorSignInSuccess(testing::_)).Times(1);
    AWSClientAuth::AuthenticationProviderNotificationBus::Broadcast(
        &AWSClientAuth::AuthenticationProviderNotifications::OnPasswordGrantSingleFactorSignInSuccess, cognitoTokens);

    // One entry max for each provider.
    ASSERT_TRUE(m_mockController->m_persistentCognitoIdentityProvider->GetLogins().size() == 1);

    AWSClientAuth::AuthenticationTokens googleTokens(
        AWSClientAuthUnitTest::TEST_TOKEN, AWSClientAuthUnitTest::TEST_TOKEN, AWSClientAuthUnitTest::TEST_TOKEN,
        AWSClientAuth::ProviderNameEnum::Google, 60);

    EXPECT_CALL(m_authenticationProviderNotificationsBusMock, OnDeviceCodeGrantConfirmSignInSuccess(testing::_)).Times(1);
    AWSClientAuth::AuthenticationProviderNotificationBus::Broadcast(
        &AWSClientAuth::AuthenticationProviderNotifications::OnDeviceCodeGrantConfirmSignInSuccess, googleTokens);    

    ASSERT_TRUE(m_mockController->m_persistentCognitoIdentityProvider->GetLogins().size() == 2);

    EXPECT_CALL(m_authenticationProviderNotificationsBusMock, OnDeviceCodeGrantConfirmSignInSuccess(testing::_)).Times(1);
    AWSClientAuth::AuthenticationProviderNotificationBus::Broadcast(
        &AWSClientAuth::AuthenticationProviderNotifications::OnDeviceCodeGrantConfirmSignInSuccess, googleTokens);

    ASSERT_TRUE(m_mockController->m_persistentCognitoIdentityProvider->GetLogins().size() == 2);

    EXPECT_CALL(m_authenticationProviderNotificationsBusMock, OnSignOut(testing::_)).Times(1);
    AWSClientAuth::AuthenticationProviderNotificationBus::Broadcast(
        &AWSClientAuth::AuthenticationProviderNotifications::OnSignOut, AWSClientAuth::ProviderNameEnum::Google);
    ASSERT_TRUE(m_mockController->m_persistentCognitoIdentityProvider->GetLogins().size() == 1);

    EXPECT_CALL(m_authenticationProviderNotificationsBusMock, OnSignOut(testing::_)).Times(1);
    AWSClientAuth::AuthenticationProviderNotificationBus::Broadcast(
        &AWSClientAuth::AuthenticationProviderNotifications::OnSignOut, AWSClientAuth::ProviderNameEnum::AWSCognitoIDP);
    ASSERT_TRUE(m_mockController->m_persistentCognitoIdentityProvider->GetLogins().size() == 0);

    AWSClientAuth::AuthenticationTokens lwaTokens(
        AWSClientAuthUnitTest::TEST_TOKEN, AWSClientAuthUnitTest::TEST_TOKEN, AWSClientAuthUnitTest::TEST_TOKEN,
        AWSClientAuth::ProviderNameEnum::LoginWithAmazon, 60);

    EXPECT_CALL(m_authenticationProviderNotificationsBusMock, OnPasswordGrantMultiFactorConfirmSignInSuccess(testing::_)).Times(1);
    AWSClientAuth::AuthenticationProviderNotificationBus::Broadcast(
        &AWSClientAuth::AuthenticationProviderNotifications::OnPasswordGrantMultiFactorConfirmSignInSuccess, lwaTokens);

    ASSERT_TRUE(m_mockController->m_persistentCognitoIdentityProvider->GetLogins().size() == 1);

    EXPECT_CALL(m_authenticationProviderNotificationsBusMock, OnSignOut(testing::_)).Times(1);
    AWSClientAuth::AuthenticationProviderNotificationBus::Broadcast(
        &AWSClientAuth::AuthenticationProviderNotifications::OnSignOut, AWSClientAuth::ProviderNameEnum::LoginWithAmazon);
    ASSERT_TRUE(m_mockController->m_persistentCognitoIdentityProvider->GetLogins().size() == 0);
}

TEST_F(AWSCognitoAuthorizationControllerTest, ResetAuthenticated_ClearsCachedLoginsAndIdentityId_Success)
{
    AWSClientAuth::AuthenticationTokens cognitoTokens(
        AWSClientAuthUnitTest::TEST_TOKEN, AWSClientAuthUnitTest::TEST_TOKEN, AWSClientAuthUnitTest::TEST_TOKEN,
        AWSClientAuth::ProviderNameEnum::AWSCognitoIDP, 60);

    EXPECT_CALL(m_authenticationProviderNotificationsBusMock, OnPasswordGrantSingleFactorSignInSuccess(testing::_)).Times(1);
    AWSClientAuth::AuthenticationProviderNotificationBus::Broadcast(
        &AWSClientAuth::AuthenticationProviderNotifications::OnPasswordGrantSingleFactorSignInSuccess, cognitoTokens);

    EXPECT_CALL(*m_cognitoIdentityClientMock, GetId(testing::_)).Times(1);
    EXPECT_CALL(*m_cognitoIdentityClientMock, GetCredentialsForIdentity(testing::_)).Times(1);
    EXPECT_CALL(m_awsCognitoAuthorizationNotificationsBusMock, OnRequestAWSCredentialsSuccess(testing::_)).Times(1);
    m_mockController->RequestAWSCredentialsAsync();

    ASSERT_TRUE(m_mockController->GetIdentityId() == AWSClientAuthUnitTest::TEST_IDENTITY_ID);
    auto creds = m_mockController->GetCognitoCredentialsProvider()->GetAWSCredentials();
    ASSERT_TRUE(creds.GetAWSAccessKeyId() == AWSClientAuthUnitTest::TEST_ACCESS_KEY_ID);
    ASSERT_TRUE(creds.GetAWSSecretKey() == AWSClientAuthUnitTest::TEST_SECRET_KEY_ID);

    m_mockController->Reset();


    EXPECT_TRUE(m_mockController->GetIdentityId() == "");
    EXPECT_TRUE(m_mockController->m_persistentCognitoIdentityProvider->GetLogins().size() == 0);

    EXPECT_CALL(*m_cognitoIdentityClientMock, GetId(testing::_)).Times(1);
    EXPECT_CALL(*m_cognitoIdentityClientMock, GetCredentialsForIdentity(testing::_)).Times(1);
    EXPECT_CALL(m_awsCognitoAuthorizationNotificationsBusMock, OnRequestAWSCredentialsSuccess(testing::_)).Times(1);
    m_mockController->RequestAWSCredentialsAsync();
}

TEST_F(AWSCognitoAuthorizationControllerTest, ResetAnonymous_ClearsCachedLoginsAndIdentityId_Success)
{
    EXPECT_CALL(*m_cognitoIdentityClientMock, GetId(testing::_)).Times(1);
    EXPECT_CALL(*m_cognitoIdentityClientMock, GetCredentialsForIdentity(testing::_)).Times(1);
    EXPECT_CALL(m_awsCognitoAuthorizationNotificationsBusMock, OnRequestAWSCredentialsSuccess(testing::_)).Times(1);
    m_mockController->RequestAWSCredentialsAsync();

    ASSERT_TRUE(m_mockController->GetIdentityId() == AWSClientAuthUnitTest::TEST_IDENTITY_ID);
    auto creds = m_mockController->GetAnonymousCognitoCredentialsProvider()->GetAWSCredentials();
    ASSERT_TRUE(creds.GetAWSAccessKeyId() == AWSClientAuthUnitTest::TEST_ACCESS_KEY_ID);
    ASSERT_TRUE(creds.GetAWSSecretKey() == AWSClientAuthUnitTest::TEST_SECRET_KEY_ID);

    m_mockController->Reset();

    EXPECT_TRUE(m_mockController->GetIdentityId() == "");

    EXPECT_CALL(*m_cognitoIdentityClientMock, GetId(testing::_)).Times(1);
    EXPECT_CALL(*m_cognitoIdentityClientMock, GetCredentialsForIdentity(testing::_)).Times(1);
    EXPECT_CALL(m_awsCognitoAuthorizationNotificationsBusMock, OnRequestAWSCredentialsSuccess(testing::_)).Times(1);
    m_mockController->RequestAWSCredentialsAsync();
}

TEST_F(AWSCognitoAuthorizationControllerTest, GetCredentialsProvider_ForPersistedLogins_ResultIsAuthenticatedCredentials)
{
    AWSClientAuth::AuthenticationTokens cognitoTokens(
        AWSClientAuthUnitTest::TEST_TOKEN, AWSClientAuthUnitTest::TEST_TOKEN, AWSClientAuthUnitTest::TEST_TOKEN,
        AWSClientAuth::ProviderNameEnum::AWSCognitoIDP, 60);

    EXPECT_CALL(m_authenticationProviderNotificationsBusMock, OnPasswordGrantSingleFactorSignInSuccess(testing::_)).Times(1);
    AWSClientAuth::AuthenticationProviderNotificationBus::Broadcast(
        &AWSClientAuth::AuthenticationProviderNotifications::OnPasswordGrantSingleFactorSignInSuccess, cognitoTokens);

    EXPECT_CALL(*m_cognitoIdentityClientMock, GetId(testing::_)).Times(1);
    EXPECT_CALL(*m_cognitoIdentityClientMock, GetCredentialsForIdentity(testing::_)).Times(1);

    std::shared_ptr<Aws::Auth::AWSCredentialsProvider> actualCredentialsProvider;
    AWSCore::AWSCredentialRequestBus::BroadcastResult(actualCredentialsProvider, &AWSCore::AWSCredentialRequests::GetCredentialsProvider);
    EXPECT_TRUE(actualCredentialsProvider == m_mockController->m_cognitoCachingCredentialsProvider);
}

TEST_F(AWSCognitoAuthorizationControllerTest, GetCredentialsProvider_NoPersistedLogins_ResultIsAnonymousCredentials)
{
    EXPECT_CALL(*m_cognitoIdentityClientMock, GetId(testing::_)).Times(1);
    EXPECT_CALL(*m_cognitoIdentityClientMock, GetCredentialsForIdentity(testing::_)).Times(1);

    std::shared_ptr<Aws::Auth::AWSCredentialsProvider> actualCredentialsProvider;
    AWSCore::AWSCredentialRequestBus::BroadcastResult(actualCredentialsProvider, &AWSCore::AWSCredentialRequests::GetCredentialsProvider);
    EXPECT_TRUE(actualCredentialsProvider == m_mockController->m_cognitoCachingAnonymousCredentialsProvider);
}

TEST_F(AWSCognitoAuthorizationControllerTest, GetCredentialsProvider_NoPersistedLogins_NoAnonymousCredentials_ResultNullPtr) // fails
{
    Aws::Client::AWSError<Aws::CognitoIdentity::CognitoIdentityErrors> error;
    error.SetExceptionName(AWSClientAuthUnitTest::TEST_EXCEPTION);
    Aws::CognitoIdentity::Model::GetIdOutcome outcome(error);

    EXPECT_CALL(*m_cognitoIdentityClientMock, GetId(testing::_)).Times(1).WillOnce(testing::Return(outcome));
    EXPECT_CALL(*m_cognitoIdentityClientMock, GetCredentialsForIdentity(testing::_)).Times(0);

    std::shared_ptr<Aws::Auth::AWSCredentialsProvider> actualCredentialsProvider;
    AZ_TEST_START_TRACE_SUPPRESSION;
    AWSCore::AWSCredentialRequestBus::BroadcastResult(
        actualCredentialsProvider, &AWSCore::AWSCredentialRequests::GetCredentialsProvider);
    AZ_TEST_STOP_TRACE_SUPPRESSION_NO_COUNT;
    EXPECT_TRUE(actualCredentialsProvider == nullptr);
}

TEST_F(
    AWSCognitoAuthorizationControllerTest,
    GetCredentialsProvider_OneThreadPersistLogins_SecondThreadGetCredentialsProvider_GetCredentialsSuccess)
{
    AZStd::vector<AZStd::thread> testThreads;
    AZStd::atomic_bool loginsAdded = false;
    AZStd::atomic_int anonymousLogin = 0;
    AZStd::atomic_int authenticatedLogin = 0;

    testThreads.emplace_back(AZStd::thread([&]() {
        AWSClientAuth::AuthenticationTokens cognitoTokens(
            AWSClientAuthUnitTest::TEST_TOKEN, AWSClientAuthUnitTest::TEST_TOKEN, AWSClientAuthUnitTest::TEST_TOKEN,
            AWSClientAuth::ProviderNameEnum::AWSCognitoIDP, 60);

        EXPECT_CALL(m_authenticationProviderNotificationsBusMock, OnPasswordGrantSingleFactorSignInSuccess(testing::_)).Times(1);
        AWSClientAuth::AuthenticationProviderNotificationBus::Broadcast(
            &AWSClientAuth::AuthenticationProviderNotifications::OnPasswordGrantSingleFactorSignInSuccess, cognitoTokens);
        loginsAdded = true;
    }));

    testThreads.emplace_back(AZStd::thread([&]() {
        std::shared_ptr<Aws::Auth::AWSCredentialsProvider> actualCredentialsProvider;
 
        EXPECT_CALL(*m_cognitoIdentityClientMock, GetId(testing::_)).Times(1);
        EXPECT_CALL(*m_cognitoIdentityClientMock, GetCredentialsForIdentity(testing::_)).Times(1);
        
        AWSCore::AWSCredentialRequestBus::BroadcastResult(
            actualCredentialsProvider, &AWSCore::AWSCredentialRequests::GetCredentialsProvider);
        if (actualCredentialsProvider == m_mockController->m_cognitoCachingAnonymousCredentialsProvider)
        {
            anonymousLogin++;
        }
        else if (actualCredentialsProvider == m_mockController->m_cognitoCachingCredentialsProvider)
        {
            authenticatedLogin++;
        }

        EXPECT_TRUE(loginsAdded ? m_mockController->HasPersistedLogins() : true);
        EXPECT_TRUE(actualCredentialsProvider != nullptr);
    }));

    for (auto& testThread : testThreads)
    {
        testThread.join();
    }
    testThreads.clear();

    AZStd::atomic_bool loginsCleared = false;
    testThreads.emplace_back(AZStd::thread([&]() {
        EXPECT_CALL(m_authenticationProviderNotificationsBusMock, OnSignOut(testing::_)).Times(1);
        AWSClientAuth::AuthenticationProviderNotificationBus::Broadcast(
            &AWSClientAuth::AuthenticationProviderNotifications::OnSignOut, AWSClientAuth::ProviderNameEnum::AWSCognitoIDP);
        loginsCleared = true;
    }));

    testThreads.emplace_back(AZStd::thread([&]() {

        std::shared_ptr<Aws::Auth::AWSCredentialsProvider> actualCredentialsProvider;
        // Can be 0, 1 depending on the previous thread order.
        EXPECT_CALL(*m_cognitoIdentityClientMock, GetId(testing::_)).Times(testing::Between(0, 1));
        EXPECT_CALL(*m_cognitoIdentityClientMock, GetCredentialsForIdentity(testing::_)).Times(testing::Between(0, 1));
        AWSCore::AWSCredentialRequestBus::BroadcastResult(
            actualCredentialsProvider, &AWSCore::AWSCredentialRequests::GetCredentialsProvider);
        EXPECT_FALSE(loginsCleared ? m_mockController->HasPersistedLogins() : false);
        EXPECT_TRUE(actualCredentialsProvider != nullptr);
        if (actualCredentialsProvider == m_mockController->m_cognitoCachingAnonymousCredentialsProvider)
        {
            anonymousLogin++;
        }
        else if (actualCredentialsProvider == m_mockController->m_cognitoCachingCredentialsProvider)
        {
            authenticatedLogin++;
        }
        EXPECT_TRUE(authenticatedLogin.load() + anonymousLogin.load() == 2);
    }));

    for (auto& testThread : testThreads)
    {
        testThread.join();
    }
}

TEST_F(AWSCognitoAuthorizationControllerTest, GetCredentialHandlerOrder_Call_AlwaysGetExpectedValue)
{
    int order;
    AWSCore::AWSCredentialRequestBus::BroadcastResult(order, &AWSCore::AWSCredentialRequests::GetCredentialHandlerOrder);
    EXPECT_EQ(order, AWSCore::CredentialHandlerOrder::COGNITO_IDENITY_POOL_CREDENTIAL_HANDLER);
}

TEST_F(AWSCognitoAuthorizationControllerTest, Initialize_Fail_GetResourceNameEmpty)
{
    EXPECT_CALL(m_awsResourceMappingRequestBusMock, GetResourceNameId(testing::_)).Times(1).WillOnce(testing::Return(""));
    EXPECT_CALL(m_awsResourceMappingRequestBusMock, GetDefaultAccountId()).Times(1);
    ASSERT_FALSE(m_mockController->Initialize());
}
