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

#include <Authentication/AuthenticationProviderInterface.h>
#include <Authentication/AuthenticationProviderTypes.h>
#include <aws/core/utils/Outcome.h>
#include <aws/cognito-idp/CognitoIdentityProviderClient.h>

namespace AWSClientAuth
{
    //! Implements AWS Cognito User pool authentication
    class AWSCognitoAuthenticationProvider
        : public AuthenticationProviderInterface
    {
    public:
        AWSCognitoAuthenticationProvider() = default;
        virtual ~AWSCognitoAuthenticationProvider() = default;

       // AuthenticationProviderInterface overrides
       bool Initialize(AZStd::weak_ptr<AZ::SettingsRegistryInterface> settingsRegistry) override;
       void PasswordGrantSingleFactorSignInAsync(const AZStd::string& username, const AZStd::string& password) override;
       void PasswordGrantMultiFactorSignInAsync(const AZStd::string& username, const AZStd::string& password) override;
       void PasswordGrantMultiFactorConfirmSignInAsync(const AZStd::string& username, const AZStd::string& confirmationCode) override;
       void DeviceCodeGrantSignInAsync() override;
       void DeviceCodeGrantConfirmSignInAsync() override;
       void RefreshTokensAsync() override;

    private:
        void InitiateAuthInternalAsync(const AZStd::string& username, const AZStd::string& password
            , AZStd::function<void(Aws::CognitoIdentityProvider::Model::InitiateAuthOutcome outcome)> outcomeCallback);
        void UpdateTokens(const Aws::CognitoIdentityProvider::Model::AuthenticationResultType& authenticationResult);

    protected:
        AZStd::string m_session;
        AZStd::string m_cognitoAppClientId;
    };

} // namespace AWSClientAuth
