/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
       bool Initialize() override;
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
