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
#include <AWSClientAuthGemMock.h>
#include <Authorization/AWSClientAuthPersistentCognitoIdentityProvider.h>


class AWSClientAuthPersistentCognitoIdentityProviderTest : public AWSClientAuthUnitTest::AWSClientAuthGemAllocatorFixture
{
protected:
    void SetUp() override
    {
        AWSClientAuthUnitTest::AWSClientAuthGemAllocatorFixture::SetUp();

    }

    void TearDown() override
    {
        AWSClientAuthUnitTest::AWSClientAuthGemAllocatorFixture::TearDown();
    }

public:
    
};

TEST_F(AWSClientAuthPersistentCognitoIdentityProviderTest, Initialize_AndPersisteIdentityId_Success)
{
    AWSClientAuth::AWSClientAuthPersistentCognitoIdentityProvider provider;
    provider.Initialize(AWSClientAuthUnitTest::TEST_ACCOUNT_ID, AWSClientAuthUnitTest::TEST_IDENTITY_POOL_ID);

    ASSERT_TRUE(provider.GetAccountId() == AWSClientAuthUnitTest::TEST_ACCOUNT_ID);
    ASSERT_TRUE(provider.GetIdentityPoolId() == AWSClientAuthUnitTest::TEST_IDENTITY_POOL_ID);

    provider.PersistIdentityId(AWSClientAuthUnitTest::TEST_IDENTITY_ID);
    ASSERT_TRUE(provider.GetIdentityId() == AWSClientAuthUnitTest::TEST_IDENTITY_ID);
}

TEST_F(AWSClientAuthPersistentCognitoIdentityProviderTest, AddRemoveLogins_Success)
{
    AWSClientAuth::AWSClientAuthPersistentCognitoIdentityProvider provider;
    provider.Initialize(AWSClientAuthUnitTest::TEST_ACCOUNT_ID, AWSClientAuthUnitTest::TEST_IDENTITY_POOL_ID);

    ASSERT_TRUE(provider.HasLogins() == false);
    Aws::Map<Aws::String, Aws::Auth::LoginAccessTokens> logins;
    Aws::Auth::LoginAccessTokens tokens;
    tokens.accessToken = "TestToken";
    logins.insert(std::pair<Aws::String, Aws::Auth::LoginAccessTokens>("TestLoginKey1", tokens));
    logins.insert(std::pair<Aws::String, Aws::Auth::LoginAccessTokens>("TestLoginKey2", tokens));
    provider.PersistLogins(logins);

    ASSERT_TRUE(provider.HasLogins() == true);
    ASSERT_TRUE(provider.GetLogins().size() == 2);
    ASSERT_TRUE(provider.GetLogins()["TestLoginKey1"].accessToken == tokens.accessToken);
    ASSERT_TRUE(provider.GetLogins()["TestLoginKey2"].accessToken == tokens.accessToken);

    provider.RemoveLogin("TestLoginKey1");
    ASSERT_TRUE(provider.HasLogins() == true);
    ASSERT_TRUE(provider.GetLogins().size() == 1);
    ASSERT_TRUE(provider.GetLogins()["TestLoginKey2"].accessToken == tokens.accessToken);

    provider.RemoveLogin("TestLoginKey2");
    ASSERT_TRUE(provider.HasLogins() == false);
    ASSERT_TRUE(provider.GetLogins().size() == 0);
}
