/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Authorization/AWSCognitoAuthorizationBus.h>
#include <Authorization/AWSClientAuthCognitoCachingAuthenticatedCredentialsProvider.h>
#include <Authorization/AWSClientAuthPersistentCognitoIdentityProvider.h>
#include <Authentication/AuthenticationProviderBus.h>
#include <Credential/AWSCredentialBus.h>
#include <aws/cognito-identity/CognitoIdentityClient.h>
#include <aws/identity-management/auth/CognitoCachingCredentialsProvider.h>

namespace AWSClientAuth
{
    //! Implements AWS Cognito Identity pool authorization.
    class AWSCognitoAuthorizationController
        : public AWSCognitoAuthorizationRequestBus::Handler
        , public AuthenticationProviderNotificationBus::Handler
        , public AWSCore::AWSCredentialRequestBus::Handler
    {
    public:
        AZ_RTTI(AWSCognitoAuthorizationController, "{0E731ED1-2F08-4B3C-9282-D452700F58D1}", IAWSCognitoAuthorizationRequests);
        AWSCognitoAuthorizationController();
        virtual ~AWSCognitoAuthorizationController();

        // AWSCognitoAuthorizationRequestsBus interface methods
        bool Initialize() override;
        void Reset() override;
        AZStd::string GetIdentityId() override;
        bool HasPersistedLogins() override;
        std::shared_ptr<Aws::Auth::AWSCredentialsProvider> GetCognitoCredentialsProvider() override;
        std::shared_ptr<Aws::Auth::AWSCredentialsProvider> GetAnonymousCognitoCredentialsProvider() override;
        void RequestAWSCredentialsAsync() override;

    protected:
        // AuthenticationProviderNotificationsBus interface. Update persistent login tokens on successful sign in.
        void OnPasswordGrantSingleFactorSignInSuccess(const AWSClientAuth::AuthenticationTokens& authenticationTokens) override;
        void OnPasswordGrantMultiFactorConfirmSignInSuccess(const AWSClientAuth::AuthenticationTokens& authenticationTokens) override;
        void OnDeviceCodeGrantConfirmSignInSuccess(const AWSClientAuth::AuthenticationTokens& authenticationTokens) override;
        void OnRefreshTokensSuccess(const AWSClientAuth::AuthenticationTokens& authenticationTokens) override;
        void OnSignOut(const ProviderNameEnum& provideName) override;

        // AWSCredentialRequestBus interface implementation
        int GetCredentialHandlerOrder() const override;
        std::shared_ptr<Aws::Auth::AWSCredentialsProvider> GetCredentialsProvider() override;

        std::shared_ptr<AWSClientAuthPersistentCognitoIdentityProvider> m_persistentCognitoIdentityProvider;
        std::shared_ptr<AWSClientAuthPersistentCognitoIdentityProvider> m_persistentAnonymousCognitoIdentityProvider;
        std::shared_ptr<AWSClientAuthCognitoCachingAuthenticatedCredentialsProvider> m_cognitoCachingCredentialsProvider;
        std::shared_ptr<AWSClientAuthCachingAnonymousCredsProvider> m_cognitoCachingAnonymousCredentialsProvider;

        AZStd::string m_cognitoIdentityPoolId;
        AZStd::string m_formattedCognitoUserPoolId;
        AZStd::string m_awsAccountId;

    private:
        void PersistLoginsAndRefreshAWSCredentials(const AuthenticationTokens& authenticationTokens);

        AZStd::string GetAuthenticationProviderId(const ProviderNameEnum& providerName);
        AZStd::mutex m_persistentCognitoIdentityProviderMutex;
        AZStd::mutex m_persistentAnonymousCognitoIdentityProviderMutex;
    };

} // namespace AWSClientAuth
