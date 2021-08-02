/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <AzCore/Debug/Trace.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <Authentication/LWAAuthenticationProvider.h>
#include <AWSClientAuthGemMock.h>

namespace AWSClientAuthUnitTest
{
    class LWAAuthenticationProviderLocalMock
        : public AWSClientAuth::LWAAuthenticationProvider
    {
    public:
        using AWSClientAuth::LWAAuthenticationProvider::m_settings;
    };
}

class LWAAuthenticationProviderTest
    : public AWSClientAuthUnitTest::AWSClientAuthGemAllocatorFixture
{
    void SetUp() override
    {
        AWSClientAuthUnitTest::AWSClientAuthGemAllocatorFixture::SetUp();
        AWSClientAuth::LWAProviderSetting::Reflect(*m_serializeContext);
        AZStd::string path = AZStd::string::format("%s/%s/awsCognitoAuthorization.setreg",
            m_testFolder->c_str(), AZ::SettingsRegistryInterface::RegistryFolder);
        CreateTestFile("awsCognitoAuthorization.setreg"
            , R"({"AWS": 
                    {
                        "LoginWithAmazon":
                        {
                            "AppClientId": "TestLWAClientId",
                            "GrantType":  "device_code",
                            "Scope": "profile",
                            "ResponseType":  "device_code",
                            "OAuthCodeURL": "https://api.amazon.com/auth/o2/create/codepair",
                            "OAuthTokensURL": "https://oauth2.googleapis.com/token"
                        }
                    } 
                })");
        m_settingsRegistry->MergeSettingsFile(path, AZ::SettingsRegistryInterface::Format::JsonMergePatch, {});

        m_lwaAuthenticationProviderLocalMock.Initialize();
    }

    void TearDown() override
    {
        AWSClientAuthUnitTest::AWSClientAuthGemAllocatorFixture::TearDown();
    }

public:
    AWSClientAuthUnitTest::LWAAuthenticationProviderLocalMock m_lwaAuthenticationProviderLocalMock;
    AWSClientAuthUnitTest::HttpRequestorRequestBusMock m_httpRequestorRequestBusMock;
};

TEST_F(LWAAuthenticationProviderTest, Initialize_Success)
{
    AWSClientAuthUnitTest::LWAAuthenticationProviderLocalMock mock;
    ASSERT_TRUE(mock.Initialize());
    ASSERT_EQ(mock.m_settings->m_appClientId, "TestLWAClientId");
}

TEST_F(LWAAuthenticationProviderTest, DeviceCodeGrantSignInAsync_Success)
{
    EXPECT_CALL(m_httpRequestorRequestBusMock, AddRequestWithHeadersAndBody(testing::_, testing::_, testing::_, testing::_, testing::_)).Times(1);
    EXPECT_CALL(m_authenticationProviderNotificationsBusMock, OnDeviceCodeGrantSignInSuccess(testing::_, testing::_, testing::_)).Times(1);
    m_lwaAuthenticationProviderLocalMock.DeviceCodeGrantSignInAsync();
}

TEST_F(LWAAuthenticationProviderTest, DeviceCodeGrantSignInAsync_RequestHttpError)
{
    EXPECT_CALL(m_httpRequestorRequestBusMock, AddRequestWithHeadersAndBody(testing::_, testing::_, testing::_, testing::_, testing::_)).Times(1)
        .WillOnce(testing::Invoke(&m_httpRequestorRequestBusMock, &AWSClientAuthUnitTest::HttpRequestorRequestBusMock::AddRequestWithHeadersAndBodyError));
    EXPECT_CALL(m_authenticationProviderNotificationsBusMock, OnDeviceCodeGrantSignInSuccess(testing::_, testing::_, testing::_)).Times(0);
    EXPECT_CALL(m_authenticationProviderNotificationsBusMock, OnDeviceCodeGrantSignInFail(testing::_)).Times(1);
    m_lwaAuthenticationProviderLocalMock.DeviceCodeGrantSignInAsync();
}

TEST_F(LWAAuthenticationProviderTest, DeviceCodeGrantConfirmSignInAsync_Success)
{
    EXPECT_CALL(m_httpRequestorRequestBusMock, AddRequestWithHeadersAndBody(testing::_, testing::_, testing::_, testing::_, testing::_)).Times(1);
    EXPECT_CALL(m_authenticationProviderNotificationsBusMock, OnDeviceCodeGrantConfirmSignInSuccess(testing::_)).Times(1);
    m_lwaAuthenticationProviderLocalMock.DeviceCodeGrantConfirmSignInAsync();
}

TEST_F(LWAAuthenticationProviderTest, DeviceCodeGrantConfirmSignInAsync_Fail_RequestHttpError)
{
    EXPECT_CALL(m_httpRequestorRequestBusMock, AddRequestWithHeadersAndBody(testing::_, testing::_, testing::_, testing::_, testing::_)).Times(1)
        .WillOnce(testing::Invoke(&m_httpRequestorRequestBusMock, &AWSClientAuthUnitTest::HttpRequestorRequestBusMock::AddRequestWithHeadersAndBodyError)); 
    EXPECT_CALL(m_authenticationProviderNotificationsBusMock, OnDeviceCodeGrantConfirmSignInSuccess(testing::_)).Times(0);
    EXPECT_CALL(m_authenticationProviderNotificationsBusMock, OnDeviceCodeGrantConfirmSignInFail(testing::_)).Times(1);
    m_lwaAuthenticationProviderLocalMock.DeviceCodeGrantConfirmSignInAsync();
}

TEST_F(LWAAuthenticationProviderTest, RefreshTokensAsync_Success)
{
    EXPECT_CALL(m_httpRequestorRequestBusMock, AddRequestWithHeadersAndBody(testing::_, testing::_, testing::_, testing::_, testing::_)).Times(1);
    EXPECT_CALL(m_authenticationProviderNotificationsBusMock, OnRefreshTokensSuccess(testing::_)).Times(1);
    m_lwaAuthenticationProviderLocalMock.RefreshTokensAsync();
}

TEST_F(LWAAuthenticationProviderTest, RefreshTokensAsync_Fail_RequestHttpError)
{
    EXPECT_CALL(m_httpRequestorRequestBusMock, AddRequestWithHeadersAndBody(testing::_, testing::_, testing::_, testing::_, testing::_)).Times(1)
        .WillOnce(testing::Invoke(&m_httpRequestorRequestBusMock, &AWSClientAuthUnitTest::HttpRequestorRequestBusMock::AddRequestWithHeadersAndBodyError));
    EXPECT_CALL(m_authenticationProviderNotificationsBusMock, OnRefreshTokensSuccess(testing::_)).Times(0);
    EXPECT_CALL(m_authenticationProviderNotificationsBusMock, OnRefreshTokensFail(testing::_)).Times(1);
    m_lwaAuthenticationProviderLocalMock.RefreshTokensAsync();
}

TEST_F(LWAAuthenticationProviderTest, Initialize_Fail_EmptyRegistry)
{
    AZ::SettingsRegistry::Unregister(m_settingsRegistry.get());

    AZStd::shared_ptr<AZ::SettingsRegistryImpl> registry = AZStd::make_shared<AZ::SettingsRegistryImpl>();
    registry->SetContext(m_serializeContext.get());
    AZ::SettingsRegistry::Register(registry.get());
    
    AWSClientAuthUnitTest::LWAAuthenticationProviderLocalMock mock;
    ASSERT_FALSE(mock.Initialize());
    ASSERT_EQ(mock.m_settings->m_appClientId, "");
    AZ::SettingsRegistry::Unregister(registry.get());
    registry.reset();

    // Restore
    AZ::SettingsRegistry::Register(m_settingsRegistry.get());
    mock.Initialize();
}
