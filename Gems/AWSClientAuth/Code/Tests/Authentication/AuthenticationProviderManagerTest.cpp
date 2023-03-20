/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/std/utils.h>
#include <Authentication/AuthenticationProviderManager.h>
#include <Authentication/AWSCognitoAuthenticationProvider.h>
#include <Authentication/LWAAuthenticationProvider.h>
#include <Authentication/AuthenticationTokens.h>
#include <AWSClientAuthGemMock.h>
#include <Authentication/AuthenticationProviderManagerMock.h>


class AuthenticationProviderManagerTest
    : public AWSClientAuthUnitTest::AWSClientAuthGemAllocatorFixture
{
protected:
    void SetUp() override
    {
        AWSClientAuthUnitTest::AWSClientAuthGemAllocatorFixture::SetUp();

        AWSClientAuth::LWAProviderSetting::Reflect(*m_serializeContext);
        AWSClientAuth::GoogleProviderSetting::Reflect(*m_serializeContext);

        AZStd::string settingspath = AZStd::string::format(
            "%s/%s/authenticationProvider.setreg",
            m_testFolder->c_str(), AZ::SettingsRegistryInterface::RegistryFolder);
        CreateTestFile("authenticationProvider.setreg"
            , R"({
                "AWS": 
                {
                    "LoginWithAmazon": 
                    {
                        "AppClientId": "TestLWAClientId",
                        "GrantType":  "device_code",
                        "Scope": "profile",
                        "ResponseType":  "device_code",
                        "OAuthCodeURL": "https://api.amazon.com/auth/o2/create/codepair",
                        "OAuthTokensURL": "https://oauth2.googleapis.com/token"
                    },
                    "Google": 
                    {
                        "AppClientId": "TestGoogleClientId",
                        "ClientSecret": "123",
                        "GrantType":  "urn:ietf:params:oauth:grant-type:device_code",
                        "Scope": "profile",
                        "OAuthCodeURL": "https://oauth2.googleapis.com/device/code",
                        "OAuthTokensURL": "https://oauth2.googleapis.com/token"
                    }
                } 
            })");
        m_settingsRegistry->MergeSettingsFile(settingspath, AZ::SettingsRegistryInterface::Format::JsonMergePatch, {});

        m_mockController = AZStd::make_unique<testing::NiceMock<AWSClientAuthUnitTest::AuthenticationProviderManagerLocalMock>>();
    }

    void TearDown() override
    {
        m_mockController.reset();
        AWSClientAuthUnitTest::AWSClientAuthGemAllocatorFixture::TearDown();
    }

public:
    AZStd::unique_ptr<testing::NiceMock<AWSClientAuthUnitTest::AuthenticationProviderManagerLocalMock>> m_mockController;
    AZStd::vector<AWSClientAuth::ProviderNameEnum> m_enabledProviderNames {AWSClientAuth::ProviderNameEnum::AWSCognitoIDP,
        AWSClientAuth::ProviderNameEnum::LoginWithAmazon, AWSClientAuth::ProviderNameEnum::Google};
};

TEST_F(AuthenticationProviderManagerTest, Initialize_Success)
{
    ASSERT_TRUE(m_mockController->Initialize(m_enabledProviderNames));
    ASSERT_TRUE(m_mockController->m_authenticationProvidersMap[AWSClientAuth::ProviderNameEnum::AWSCognitoIDP] != nullptr);
}

TEST_F(AuthenticationProviderManagerTest, PasswordGrantSingleFactorSignInAsync_Success)
{
    m_mockController->Initialize(m_enabledProviderNames);
    testing::NiceMock<AWSClientAuthUnitTest::AuthenticationProviderMock> *cognitoProviderMock = (testing::NiceMock<AWSClientAuthUnitTest::AuthenticationProviderMock>*)m_mockController->m_authenticationProvidersMap[AWSClientAuth::ProviderNameEnum::AWSCognitoIDP].get();
  
    EXPECT_CALL(*cognitoProviderMock, PasswordGrantSingleFactorSignInAsync(testing::_, testing::_)).Times(1);
    m_mockController->PasswordGrantSingleFactorSignInAsync(AWSClientAuth::ProviderNameEnum::AWSCognitoIDP, AWSClientAuthUnitTest::TEST_USERNAME, AWSClientAuthUnitTest::TEST_PASSWORD);
    cognitoProviderMock = nullptr;
}

TEST_F(AuthenticationProviderManagerTest, PasswordGrantSingleFactorSignInAsync_Fail_NonConfiguredProviderError)
{
    AZ_TEST_START_TRACE_SUPPRESSION;
    m_mockController->PasswordGrantSingleFactorSignInAsync(AWSClientAuth::ProviderNameEnum::Apple, AWSClientAuthUnitTest::TEST_USERNAME, AWSClientAuthUnitTest::TEST_PASSWORD);
    AZ_TEST_STOP_TRACE_SUPPRESSION(1);
}

TEST_F(AuthenticationProviderManagerTest, PasswordGrantMultiFactorSignInAsync_Success)
{
    m_mockController->Initialize(m_enabledProviderNames);
    testing::NiceMock<AWSClientAuthUnitTest::AuthenticationProviderMock>* cognitoProviderMock = (testing::NiceMock<AWSClientAuthUnitTest::AuthenticationProviderMock>*)m_mockController->m_authenticationProvidersMap[AWSClientAuth::ProviderNameEnum::AWSCognitoIDP].get();
    testing::NiceMock<AWSClientAuthUnitTest::AuthenticationProviderMock>* lwaProviderMock = (testing::NiceMock<AWSClientAuthUnitTest::AuthenticationProviderMock>*)m_mockController->m_authenticationProvidersMap[AWSClientAuth::ProviderNameEnum::LoginWithAmazon].get();

    EXPECT_CALL(*cognitoProviderMock, PasswordGrantMultiFactorSignInAsync(testing::_, testing::_)).Times(1);
    m_mockController->PasswordGrantMultiFactorSignInAsync(AWSClientAuth::ProviderNameEnum::AWSCognitoIDP, AWSClientAuthUnitTest::TEST_USERNAME, AWSClientAuthUnitTest::TEST_PASSWORD);

    EXPECT_CALL(*lwaProviderMock, PasswordGrantMultiFactorSignInAsync(testing::_, testing::_)).Times(1);
    m_mockController->PasswordGrantMultiFactorSignInAsync(AWSClientAuth::ProviderNameEnum::LoginWithAmazon, AWSClientAuthUnitTest::TEST_USERNAME, AWSClientAuthUnitTest::TEST_PASSWORD);

    cognitoProviderMock = nullptr;
}

TEST_F(AuthenticationProviderManagerTest, PasswordGrantMultiFactorConfirmSignInAsync_Success)
{
    m_mockController->Initialize(m_enabledProviderNames);
    testing::NiceMock<AWSClientAuthUnitTest::AuthenticationProviderMock> *cognitoProviderMock = (testing::NiceMock<AWSClientAuthUnitTest::AuthenticationProviderMock>*)m_mockController->m_authenticationProvidersMap[AWSClientAuth::ProviderNameEnum::AWSCognitoIDP].get();
    testing::NiceMock<AWSClientAuthUnitTest::AuthenticationProviderMock> *lwaProviderMock = (testing::NiceMock<AWSClientAuthUnitTest::AuthenticationProviderMock>*)m_mockController->m_authenticationProvidersMap[AWSClientAuth::ProviderNameEnum::LoginWithAmazon].get();

    EXPECT_CALL(*cognitoProviderMock, PasswordGrantMultiFactorConfirmSignInAsync(testing::_, testing::_)).Times(1);
    m_mockController->PasswordGrantMultiFactorConfirmSignInAsync(AWSClientAuth::ProviderNameEnum::AWSCognitoIDP, AWSClientAuthUnitTest::TEST_USERNAME, AWSClientAuthUnitTest::TEST_PASSWORD);

    EXPECT_CALL(*lwaProviderMock, PasswordGrantMultiFactorConfirmSignInAsync(testing::_, testing::_)).Times(1);
    m_mockController->PasswordGrantMultiFactorConfirmSignInAsync(AWSClientAuth::ProviderNameEnum::LoginWithAmazon, AWSClientAuthUnitTest::TEST_USERNAME, AWSClientAuthUnitTest::TEST_PASSWORD);

    cognitoProviderMock = nullptr;
}

TEST_F(AuthenticationProviderManagerTest, DeviceCodeGrantSignInAsync_Success)
{
    m_mockController->Initialize(m_enabledProviderNames);
    testing::NiceMock<AWSClientAuthUnitTest::AuthenticationProviderMock>* cognitoProviderMock = (testing::NiceMock<AWSClientAuthUnitTest::AuthenticationProviderMock>*)m_mockController->m_authenticationProvidersMap[AWSClientAuth::ProviderNameEnum::AWSCognitoIDP].get();
    testing::NiceMock<AWSClientAuthUnitTest::AuthenticationProviderMock>* lwaProviderMock = (testing::NiceMock<AWSClientAuthUnitTest::AuthenticationProviderMock>*)m_mockController->m_authenticationProvidersMap[AWSClientAuth::ProviderNameEnum::LoginWithAmazon].get();

    EXPECT_CALL(*cognitoProviderMock, DeviceCodeGrantSignInAsync()).Times(1);
    m_mockController->DeviceCodeGrantSignInAsync(AWSClientAuth::ProviderNameEnum::AWSCognitoIDP);

    EXPECT_CALL(*lwaProviderMock, DeviceCodeGrantSignInAsync()).Times(1);
    m_mockController->DeviceCodeGrantSignInAsync(AWSClientAuth::ProviderNameEnum::LoginWithAmazon);

    cognitoProviderMock = nullptr;
}


TEST_F(AuthenticationProviderManagerTest, DeviceCodeGrantConfirmSignInAsync_Success)
{
    m_mockController->Initialize(m_enabledProviderNames);
    testing::NiceMock<AWSClientAuthUnitTest::AuthenticationProviderMock>* cognitoProviderMock = (testing::NiceMock<AWSClientAuthUnitTest::AuthenticationProviderMock>*)m_mockController->m_authenticationProvidersMap[AWSClientAuth::ProviderNameEnum::AWSCognitoIDP].get();
    testing::NiceMock<AWSClientAuthUnitTest::AuthenticationProviderMock>* lwaProviderMock = (testing::NiceMock<AWSClientAuthUnitTest::AuthenticationProviderMock>*)m_mockController->m_authenticationProvidersMap[AWSClientAuth::ProviderNameEnum::LoginWithAmazon].get();

    EXPECT_CALL(*cognitoProviderMock, DeviceCodeGrantConfirmSignInAsync()).Times(1);
    m_mockController->DeviceCodeGrantConfirmSignInAsync(AWSClientAuth::ProviderNameEnum::AWSCognitoIDP);

    EXPECT_CALL(*lwaProviderMock, DeviceCodeGrantConfirmSignInAsync()).Times(1);
    m_mockController->DeviceCodeGrantConfirmSignInAsync(AWSClientAuth::ProviderNameEnum::LoginWithAmazon);

    cognitoProviderMock = nullptr;
}

TEST_F(AuthenticationProviderManagerTest, RefreshTokenAsync_Success)
{
    m_mockController->Initialize(m_enabledProviderNames);
    testing::NiceMock<AWSClientAuthUnitTest::AuthenticationProviderMock> *cognitoProviderMock = (testing::NiceMock<AWSClientAuthUnitTest::AuthenticationProviderMock>*)m_mockController->m_authenticationProvidersMap[AWSClientAuth::ProviderNameEnum::AWSCognitoIDP].get();
    testing::NiceMock<AWSClientAuthUnitTest::AuthenticationProviderMock> *lwaProviderMock = (testing::NiceMock<AWSClientAuthUnitTest::AuthenticationProviderMock>*)m_mockController->m_authenticationProvidersMap[AWSClientAuth::ProviderNameEnum::LoginWithAmazon].get();

    EXPECT_CALL(*cognitoProviderMock, RefreshTokensAsync()).Times(1);
    m_mockController->RefreshTokensAsync(AWSClientAuth::ProviderNameEnum::AWSCognitoIDP);

    EXPECT_CALL(*lwaProviderMock, RefreshTokensAsync()).Times(1);
    m_mockController->RefreshTokensAsync(AWSClientAuth::ProviderNameEnum::LoginWithAmazon);

    cognitoProviderMock = nullptr;
}

TEST_F(AuthenticationProviderManagerTest, GetTokensWithRefreshAsync_ValidToken_Success)
{
    m_mockController->Initialize(m_enabledProviderNames);
    testing::NiceMock<AWSClientAuthUnitTest::AuthenticationProviderMock>* cognitoProviderMock = (testing::NiceMock<AWSClientAuthUnitTest::AuthenticationProviderMock>*)m_mockController->m_authenticationProvidersMap[AWSClientAuth::ProviderNameEnum::AWSCognitoIDP].get();

     AWSClientAuth::AuthenticationTokens tokens(
        AWSClientAuthUnitTest::TEST_ACCESS_TOKEN, AWSClientAuthUnitTest::TEST_REFRESH_TOKEN, AWSClientAuthUnitTest::TEST_ID_TOKEN,
        AWSClientAuth::ProviderNameEnum::AWSCognitoIDP, 600);
    EXPECT_CALL(*cognitoProviderMock, GetAuthenticationTokens()).Times(1).WillOnce(testing::Return(tokens));
    EXPECT_CALL(*cognitoProviderMock, RefreshTokensAsync()).Times(0);
    EXPECT_CALL(m_authenticationProviderNotificationsBusMock, OnRefreshTokensSuccess(testing::_)).Times(1);
    m_mockController->GetTokensWithRefreshAsync(AWSClientAuth::ProviderNameEnum::AWSCognitoIDP);

    cognitoProviderMock = nullptr;
}

TEST_F(AuthenticationProviderManagerTest, GetTokensWithRefreshAsync_InvalidToken_Success)
{
    m_mockController->Initialize(m_enabledProviderNames);
    testing::NiceMock<AWSClientAuthUnitTest::AuthenticationProviderMock>* cognitoProviderMock = (testing::NiceMock<AWSClientAuthUnitTest::AuthenticationProviderMock>*)m_mockController->m_authenticationProvidersMap[AWSClientAuth::ProviderNameEnum::AWSCognitoIDP].get();
    AWSClientAuth::AuthenticationTokens tokens;
    EXPECT_CALL(*cognitoProviderMock, GetAuthenticationTokens()).Times(1).WillOnce(testing::Return(tokens));
    EXPECT_CALL(*cognitoProviderMock, RefreshTokensAsync()).Times(1);
    m_mockController->GetTokensWithRefreshAsync(AWSClientAuth::ProviderNameEnum::AWSCognitoIDP);

    cognitoProviderMock = nullptr;
}

TEST_F(AuthenticationProviderManagerTest, GetTokensWithRefreshAsync_NotInitializedProvider_Fail)
{
    AZ_TEST_START_TRACE_SUPPRESSION;
    EXPECT_CALL(m_authenticationProviderNotificationsBusMock, OnRefreshTokensSuccess(testing::_)).Times(0);
    EXPECT_CALL(m_authenticationProviderNotificationsBusMock, OnRefreshTokensFail(testing::_)).Times(1);
    m_mockController->GetTokensWithRefreshAsync(AWSClientAuth::ProviderNameEnum::AWSCognitoIDP);
    AZ_TEST_STOP_TRACE_SUPPRESSION(1);
}

TEST_F(AuthenticationProviderManagerTest, GetTokens_Success)
{
    m_mockController->Initialize(m_enabledProviderNames);
    testing::NiceMock<AWSClientAuthUnitTest::AuthenticationProviderMock>* cognitoProviderMock = (testing::NiceMock<AWSClientAuthUnitTest::AuthenticationProviderMock>*)m_mockController->m_authenticationProvidersMap[AWSClientAuth::ProviderNameEnum::AWSCognitoIDP].get();

     AWSClientAuth::AuthenticationTokens tokens(
        AWSClientAuthUnitTest::TEST_ACCESS_TOKEN, AWSClientAuthUnitTest::TEST_REFRESH_TOKEN, AWSClientAuthUnitTest::TEST_ID_TOKEN,
        AWSClientAuth::ProviderNameEnum::AWSCognitoIDP, 60);

    EXPECT_CALL(*cognitoProviderMock, GetAuthenticationTokens()).Times(1).WillOnce(testing::Return(tokens));
    m_mockController->GetAuthenticationTokens(AWSClientAuth::ProviderNameEnum::AWSCognitoIDP);

    cognitoProviderMock = nullptr;
}

TEST_F(AuthenticationProviderManagerTest, IsSignedIn_Success)
{
    m_mockController->Initialize(m_enabledProviderNames);
    testing::NiceMock<AWSClientAuthUnitTest::AuthenticationProviderMock>* cognitoProviderMock = (testing::NiceMock<AWSClientAuthUnitTest::AuthenticationProviderMock>*)m_mockController->m_authenticationProvidersMap[AWSClientAuth::ProviderNameEnum::AWSCognitoIDP].get();

     AWSClientAuth::AuthenticationTokens tokens(
        AWSClientAuthUnitTest::TEST_ACCESS_TOKEN, AWSClientAuthUnitTest::TEST_REFRESH_TOKEN, AWSClientAuthUnitTest::TEST_ID_TOKEN,
        AWSClientAuth::ProviderNameEnum::AWSCognitoIDP, 60);
    EXPECT_CALL(*cognitoProviderMock, GetAuthenticationTokens()).Times(1).WillOnce(testing::Return(tokens));
    m_mockController->IsSignedIn(AWSClientAuth::ProviderNameEnum::AWSCognitoIDP);

    cognitoProviderMock = nullptr;
}

TEST_F(AuthenticationProviderManagerTest, SignOut_Success)
{
    m_mockController->Initialize(m_enabledProviderNames);
    testing::NiceMock<AWSClientAuthUnitTest::AuthenticationProviderMock>* googleProviderMock = (testing::NiceMock<AWSClientAuthUnitTest::AuthenticationProviderMock>*)m_mockController->m_authenticationProvidersMap[AWSClientAuth::ProviderNameEnum::Google].get();

    EXPECT_CALL(*googleProviderMock, SignOut()).Times(1);
    EXPECT_CALL(m_authenticationProviderNotificationsBusMock, OnSignOut(testing::_)).Times(1);
    m_mockController->SignOut(AWSClientAuth::ProviderNameEnum::Google);

    googleProviderMock = nullptr;
}

