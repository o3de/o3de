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

        m_googleAuthenticationProviderLocalMock.Initialize(m_settingsRegistry);
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
    ASSERT_TRUE(mock.Initialize(m_settingsRegistry));
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
    AZStd::shared_ptr<AZ::SettingsRegistryImpl> registry = AZStd::make_shared<AZ::SettingsRegistryImpl>();
    registry->SetContext(m_serializeContext.get());
    
    AWSClientAuthUnitTest::GoogleAuthenticationProviderLocalMock mock;
    ASSERT_FALSE(mock.Initialize(registry));
    ASSERT_EQ(mock.m_settings->m_appClientId, "");
    registry.reset();

    // Restore
    mock.Initialize(m_settingsRegistry);
}
