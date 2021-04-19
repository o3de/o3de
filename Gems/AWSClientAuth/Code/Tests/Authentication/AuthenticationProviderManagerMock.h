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

#pragma once

#include <AWSClientAuthGemMock.h>
#include <Authentication/AuthenticationProviderManager.h>

namespace AWSClientAuthUnitTest
{
    class AuthenticationProviderManagerLocalMock
        : public AWSClientAuth::AuthenticationProviderManager
    {
    public:
        using AWSClientAuth::AuthenticationProviderManager::DeviceCodeGrantConfirmSignInAsync;
        using AWSClientAuth::AuthenticationProviderManager::DeviceCodeGrantSignInAsync;
        using AWSClientAuth::AuthenticationProviderManager::GetAuthenticationTokens;
        using AWSClientAuth::AuthenticationProviderManager::GetTokensWithRefreshAsync;
        using AWSClientAuth::AuthenticationProviderManager::Initialize;
        using AWSClientAuth::AuthenticationProviderManager::IsSignedIn;
        using AWSClientAuth::AuthenticationProviderManager::m_authenticationProvidersMap;
        using AWSClientAuth::AuthenticationProviderManager::PasswordGrantMultiFactorConfirmSignInAsync;
        using AWSClientAuth::AuthenticationProviderManager::PasswordGrantMultiFactorSignInAsync;
        using AWSClientAuth::AuthenticationProviderManager::PasswordGrantSingleFactorSignInAsync;
        using AWSClientAuth::AuthenticationProviderManager::RefreshTokensAsync;
        using AWSClientAuth::AuthenticationProviderManager::SignOut;

        AZStd::unique_ptr<AWSClientAuth::AuthenticationProviderInterface> CreateAuthenticationProviderObjectMock(
            const AWSClientAuth::ProviderNameEnum& providerName)
        {
            auto providerObject = AWSClientAuth::AuthenticationProviderManager::CreateAuthenticationProviderObject(providerName);
            providerObject.reset();
            return AZStd::make_unique<testing::NiceMock<AWSClientAuthUnitTest::AuthenticationProviderMock>>();
        }

        AuthenticationProviderManagerLocalMock()
        {
            ON_CALL(*this, CreateAuthenticationProviderObject(testing::_))
                .WillByDefault(testing::Invoke(this, &AuthenticationProviderManagerLocalMock::CreateAuthenticationProviderObjectMock));
        }

        MOCK_METHOD1(
            CreateAuthenticationProviderObject,
            AZStd::unique_ptr<AWSClientAuth::AuthenticationProviderInterface>(const AWSClientAuth::ProviderNameEnum&));
    };
} // namespace AWSClientAuthUnitTest
