/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/EBus/EBus.h>
#include <Authentication/AuthenticationTokens.h>

namespace AWSClientAuth
{
    //! Abstract class for authentication provider requests.
    class IAuthenticationProviderRequests
    {
    public:
        AZ_TYPE_INFO(IAuthenticationProviderRequests, "{4A8017C4-2742-48C4-AF07-1177CBF5E6E9}");

        //! Parse the settings file for required settings for authentication providers. Instantiate and initialize authentication providers
        //! @param providerNames List of provider names to instantiate and initialize for Authentication.
        //! @return bool True: if all providers initialized successfully. False: If any provider fails initialization.
        virtual bool Initialize(const AZStd::vector<ProviderNameEnum>& providerNames) = 0;

        //! Checks if user is signed in.
        //! If access tokens are available and not expired.
        //! @param providerName Provider to check signed in for
        //! @return bool True if valid access token available, else False
        virtual bool IsSignedIn(const ProviderNameEnum& providerName) = 0;

        //! Get cached tokens from last last successful sign-in for the provider.
        //! @param providerName Provider to get authentication tokens.
        //! @return AuthenticationTokens tokens from successful authentication.
        virtual AuthenticationTokens GetAuthenticationTokens(const ProviderNameEnum& providerName) = 0;

        // Below methods have corresponding notifications for success and failures.

        //! Call sign in endpoint for provider password grant flow.
        //! @param providerName Provider to call sign in.
        //! @param username Username to use to for sign in.
        //! @param password Password to use to for sign in.
        virtual void PasswordGrantSingleFactorSignInAsync(const ProviderNameEnum& providerName, const AZStd::string& username, const AZStd::string& password) = 0;

        //! Call sign in endpoint for provider password grant multi factor authentication flow.
        //! @param providerName Provider to call MFA sign in.
        //! @param username Username to use for MFA sign in.
        //! @param password Password to use for MFA sign in.
        virtual void PasswordGrantMultiFactorSignInAsync(const ProviderNameEnum& providerName, const AZStd::string& username, const AZStd::string& password) = 0;

        //! Call confirm endpoint for provider password grant multi factor authentication flow .
        //! @param providerName Provider to call MFA confirm sign in.
        //! @param username Username to use for MFA confirm.
        //! @param confirmationCode Confirmation code (sent to email/text) to use for MFA confirm.
        virtual void PasswordGrantMultiFactorConfirmSignInAsync(const ProviderNameEnum& providerName, const AZStd::string& username, const AZStd::string& confirmationCode) = 0;

        //! Call code-pair endpoint for provider device grant flow.
        //! @param providerName Provider to call device sign in.
        virtual void DeviceCodeGrantSignInAsync(const ProviderNameEnum& providerName) = 0;

        //! Call tokens endpoint for provider device grant flow.
        //! @param providerName Provider to call device confirm sign in.
        virtual void DeviceCodeGrantConfirmSignInAsync(const ProviderNameEnum& providerName) = 0;

        //! Call refresh endpoint for provider refresh grant flow.
        //! @param providerName Provider to call refresh tokens.
        virtual void RefreshTokensAsync(const ProviderNameEnum& providerName) = 0;

        //! Call refresh token if token not valid. If token valid, fires corresponding event.
        //! @param providerName Provider to get access token for.
        //! events: OnRefreshTokensSuccess, OnRefreshTokensFail
        virtual void GetTokensWithRefreshAsync(const ProviderNameEnum& providerName) = 0;

        //! Signs user out.
        //! Clears all cached tokens.
        //! @param providerName Provider to sign out.
        //! @return bool True: Successfully sign out.
        virtual bool SignOut(const ProviderNameEnum& providerName) = 0;

        //////////////////////////////////////////////////////////////////////////
    };

    //! Authentication Request bus for different supported providers.
    class AuthenticationProviderRequests
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        using MutexType = AZ::NullMutex;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        //////////////////////////////////////////////////////////////////////////
    };
    using AuthenticationProviderRequestBus = AZ::EBus<IAuthenticationProviderRequests, AuthenticationProviderRequests>;


    //! Notification bus for Authentication Request bus.
    class AuthenticationProviderNotifications
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        //////////////////////////////////////////////////////////////////////////

        ////////////////////////////ss//////////////////////////////////////////////
        // Notifications interface

        //! Event for PasswordGrantSingleFactorSignIn success.
        //! @param authenticationToken Tokens on successful sign in.
        virtual void OnPasswordGrantSingleFactorSignInSuccess(const AuthenticationTokens& authenticationToken)
        {
            AZ_UNUSED(authenticationToken);
        }

        //! Event for PasswordGrantSingleFactorSignIn fail.
        //! @param error Error message
        virtual void OnPasswordGrantSingleFactorSignInFail(const AZStd::string& error)
        {
            AZ_UNUSED(error);
        }

        //! Event for PasswordGrantMultiFactorSignIn success.
        //! Event use to notify user to take corresponding challenge action.
        virtual void OnPasswordGrantMultiFactorSignInSuccess()
        {
        }

        //! Event for PasswordGrantMultiFactorSignIn fail.
        //! @param error Error message
        virtual void OnPasswordGrantMultiFactorSignInFail(const AZStd::string& error)
        {
            AZ_UNUSED(error);
        }

        //! Event for PasswordGrantMultiFactorConfirm success.
        //! @param authenticationToken Tokens on successful sign in.
        virtual void OnPasswordGrantMultiFactorConfirmSignInSuccess(const AuthenticationTokens& authenticationToken)
        {
            AZ_UNUSED(authenticationToken);
        }

        //! Event for PasswordGrantMultiFactorConfirm fail.
        //! @param error Error message
        virtual void OnPasswordGrantMultiFactorConfirmSignInFail(const AZStd::string& error)
        {
            AZ_UNUSED(error);
        }

        //! Event for DeviceCodeGrantSignIn success.
        //! Event use to notify user to take open verification url and enter displayed code.
        //! @param userCode Unique code generated for user for the session.
        //! @param verificationUrl Verification URL to enter user code in after signing in for the provider.
        //! @param codeExpiresInSeconds Code expiry in seconds.
        virtual void OnDeviceCodeGrantSignInSuccess(const AZStd::string& userCode, const AZStd::string& verificationUrl, int codeExpiresInSeconds)
        {
            AZ_UNUSED(userCode);
            AZ_UNUSED(verificationUrl);
            AZ_UNUSED(codeExpiresInSeconds);
        }

        //! Event for DeviceCodeGrantSignIn fail.
        //! @param error Error message
        virtual void OnDeviceCodeGrantSignInFail(const AZStd::string& error)
        {
            AZ_UNUSED(error);
        }

        //! Event for DeviceCodeGrantConfirmSignIn success.
        //! @param authenticationToken Tokens on successful sign in..
        virtual void OnDeviceCodeGrantConfirmSignInSuccess(const AuthenticationTokens& authenticationToken)
        {
            AZ_UNUSED(authenticationToken);
        }

        //! Event for DeviceCodeGrantConfirmSignIn fail.
        //! @param error Error message
        virtual void OnDeviceCodeGrantConfirmSignInFail(const AZStd::string& error)
        {
            AZ_UNUSED(error);
        }

        //! Event for RequestAccessTokenWithRefresh success.
        //! @param authenticationToken Tokens on successful sign in.
        virtual void OnRefreshTokensSuccess(const AuthenticationTokens& authenticationToken)
        {
            AZ_UNUSED(authenticationToken);
        }

        //! Event for RequestAccessTokenWithRefresh fail.
        //! @param error Error message
        virtual void OnRefreshTokensFail(const AZStd::string& error)
        {
            AZ_UNUSED(error);
        }

        //! Event for Sing out.
        //! @param providerName provider that signed out.
        virtual void OnSignOut(const ProviderNameEnum& provideName)
        {
            AZ_UNUSED(provideName);
        }

        //////////////////////////////////////////////////////////////////////////

    };
    using AuthenticationProviderNotificationBus = AZ::EBus<AuthenticationProviderNotifications>;
} // namespace AWSClientAuth
