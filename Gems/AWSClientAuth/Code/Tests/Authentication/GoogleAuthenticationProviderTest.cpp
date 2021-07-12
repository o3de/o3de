/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <AzCore/Debug/Trace.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <Authentication/GoogleAuthenticationProvider.h>
#include <AWSClientAuthGemMock.h>

namespace AWSClientAuthUnitTest
{
    class GoogleAuthenticationProviderLocalMock
        : public AWSClientAuth::GoogleAuthenticationProvider
    {
    public:
        using AWSClientAuth::GoogleAuthenticationProvider::m_settings;
    };
}

class GoogleAuthenticationProviderTest
    : public AWSClientAuthUnitTest::AWSClientAuthGemAllocatorFixture
{
    void SetUp() override
    {
        AWSClientAuthUnitTest::AWSClientAuthGemAllocatorFixture::SetUp();
        AWSClientAuth::GoogleProviderSetting::Reflect(*m_serializeContext);
        AZStd::string path = AZStd::string::format("%s/%s/awsCognitoAuthorization.setreg",
            m_testFolder->c_str(), AZ::SettingsRegistryInterface::RegistryFolder);
        CreateTestFile("awsCognitoAuthorization.setreg"
            , R"({"AWS": 
                    {
                        "Google": 
                        {
                            "AppClientId": "TestGoogleClientId",
                            "ClientSecret": "TestClientSecret",
                            "GrantType":  "urn:ietf:params:oauth:grant-type:device_code",
                            "Scope": "profile",
                            "OAuthCodeURL": "https://oauth2.googleapis.com/device/code",
                            "OAuthTokensURL": "https://oauth2.googleapis.com/token"
                        }
                    } 
                })");
        m_settingsRegistry->MergeSettingsFile(path, AZ::SettingsRegistryInterface::Format::JsonMergePatch, {});

        m_googleAuthenticationProviderLocalMock.Initialize();
    }

    void TearDown() override
    {
        AWSClientAuthUnitTest::AWSClientAuthGemAllocatorFixture::TearDown();
    }

public:
    AWSClientAuthUnitTest::GoogleAuthenticationProviderLocalMock m_googleAuthenticationProviderLocalMock;
    AWSClientAuthUnitTest::HttpRequestorRequestBusMock m_httpRequestorRequestBusMock;
};

TEST_F(GoogleAuthenticationProviderTest, Initialize_Success)
{
    AWSClientAuthUnitTest::GoogleAuthenticationProviderLocalMock mock;
    ASSERT_TRUE(mock.Initialize());
    ASSERT_EQ(mock.m_settings->m_appClientId, "TestGoogleClientId");
}

TEST_F(GoogleAuthenticationProviderTest, DeviceCodeGrantSignInAsync_Success)
{
    EXPECT_CALL(m_httpRequestorRequestBusMock, AddRequestWithHeadersAndBody(testing::_, testing::_, testing::_, testing::_, testing::_)).Times(1);
    EXPECT_CALL(m_authenticationProviderNotificationsBusMock, OnDeviceCodeGrantSignInSuccess(testing::_, testing::_, testing::_)).Times(1);
    m_googleAuthenticationProviderLocalMock.DeviceCodeGrantSignInAsync();
}

TEST_F(GoogleAuthenticationProviderTest, DeviceCodeGrantSignInAsync_Fail_RequestHttpError)
{
    EXPECT_CALL(m_httpRequestorRequestBusMock, AddRequestWithHeadersAndBody(testing::_, testing::_, testing::_, testing::_, testing::_)).Times(1)
        .WillOnce(testing::Invoke(&m_httpRequestorRequestBusMock, &AWSClientAuthUnitTest::HttpRequestorRequestBusMock::AddRequestWithHeadersAndBodyError));
    EXPECT_CALL(m_authenticationProviderNotificationsBusMock, OnDeviceCodeGrantSignInSuccess(testing::_, testing::_, testing::_)).Times(0);
    EXPECT_CALL(m_authenticationProviderNotificationsBusMock, OnDeviceCodeGrantSignInFail(testing::_)).Times(1);
    m_googleAuthenticationProviderLocalMock.DeviceCodeGrantSignInAsync();
}

TEST_F(GoogleAuthenticationProviderTest, DeviceCodeGrantConfirmAsync_Success)
{
    EXPECT_CALL(m_httpRequestorRequestBusMock, AddRequestWithHeadersAndBody(testing::_, testing::_, testing::_, testing::_, testing::_)).Times(1);
    EXPECT_CALL(m_authenticationProviderNotificationsBusMock, OnDeviceCodeGrantConfirmSignInSuccess(testing::_)).Times(1);
    m_googleAuthenticationProviderLocalMock.DeviceCodeGrantConfirmSignInAsync();
}

TEST_F(GoogleAuthenticationProviderTest, DeviceCodeGrantConfirmSignInAsync_Fail_RequestHttpError)
{
    EXPECT_CALL(m_httpRequestorRequestBusMock, AddRequestWithHeadersAndBody(testing::_, testing::_, testing::_, testing::_, testing::_)).Times(1)
        .WillOnce(testing::Invoke(&m_httpRequestorRequestBusMock, &AWSClientAuthUnitTest::HttpRequestorRequestBusMock::AddRequestWithHeadersAndBodyError));
    EXPECT_CALL(m_authenticationProviderNotificationsBusMock, OnDeviceCodeGrantConfirmSignInSuccess(testing::_)).Times(0);
    EXPECT_CALL(m_authenticationProviderNotificationsBusMock, OnDeviceCodeGrantConfirmSignInFail(testing::_)).Times(1);
    m_googleAuthenticationProviderLocalMock.DeviceCodeGrantConfirmSignInAsync();
}

TEST_F(GoogleAuthenticationProviderTest, RefreshTokensAsync_Success)
{
    EXPECT_CALL(m_httpRequestorRequestBusMock, AddRequestWithHeadersAndBody(testing::_, testing::_, testing::_, testing::_, testing::_)).Times(1);
    EXPECT_CALL(m_authenticationProviderNotificationsBusMock, OnRefreshTokensSuccess(testing::_)).Times(1);
    m_googleAuthenticationProviderLocalMock.RefreshTokensAsync();
}

TEST_F(GoogleAuthenticationProviderTest, RefreshTokensAsync_Fail_RequestHttpError)
{
    EXPECT_CALL(m_httpRequestorRequestBusMock, AddRequestWithHeadersAndBody(testing::_, testing::_, testing::_, testing::_, testing::_)).Times(1)
        .WillOnce(testing::Invoke(&m_httpRequestorRequestBusMock, &AWSClientAuthUnitTest::HttpRequestorRequestBusMock::AddRequestWithHeadersAndBodyError));
    EXPECT_CALL(m_authenticationProviderNotificationsBusMock, OnRefreshTokensSuccess(testing::_)).Times(0);
    EXPECT_CALL(m_authenticationProviderNotificationsBusMock, OnRefreshTokensFail(testing::_)).Times(1);
    m_googleAuthenticationProviderLocalMock.RefreshTokensAsync();
}

TEST_F(GoogleAuthenticationProviderTest, Initialize_Fail_EmptyRegistry)
{
    AZ::SettingsRegistry::Unregister(m_settingsRegistry.get());

    AZStd::shared_ptr<AZ::SettingsRegistryImpl> registry = AZStd::make_shared<AZ::SettingsRegistryImpl>();
    registry->SetContext(m_serializeContext.get());
    AZ::SettingsRegistry::Register(registry.get());
    
    AWSClientAuthUnitTest::GoogleAuthenticationProviderLocalMock mock;
    ASSERT_FALSE(mock.Initialize());
    ASSERT_EQ(mock.m_settings->m_appClientId, "");
    AZ::SettingsRegistry::Unregister(registry.get());
    registry.reset();

    // Restore
    AZ::SettingsRegistry::Register(m_settingsRegistry.get());
    mock.Initialize();
}
