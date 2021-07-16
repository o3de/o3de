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
    //! Abstract class for authentication provider script canvas requests.
    //! Private class to allow provide names to be string type instead of an enum as behavior context does not work well with enum's.
    class IAuthenticationProviderScriptCanvasRequests
    {
    public:
        AZ_TYPE_INFO(IAuthenticationProviderRequests, "{A8FD915F-9FF2-4BA3-8AA0-8CF7A94A323B}");

        //! Parse the settings file for required settings for authentication providers. Instantiate and initialize authentication providers
        //! @param providerNames List of provider names to instantiate and initialize for Authentication.
        //! @return bool True: if all providers initialized successfully. False: If any provider fails initialization.
        virtual bool Initialize(const AZStd::vector<AZStd::string>& providerNames) = 0;

        //! Checks if user is signed in.
        //! If access tokens are available and not expired.
        //! @param providerName Provider to check signed in for
        //! @return bool True if valid access token available, else False
        virtual bool IsSignedIn(const AZStd::string& providerName) = 0;

        //! Get cached tokens from last last successful sign-in for the provider.
        //! @param providerName Provider to get authentication tokens
        //! @return AuthenticationTokens tokens from successful authentication.
        virtual AuthenticationTokens GetAuthenticationTokens(const AZStd::string& providerName) = 0;

        // Below methods have corresponding notifications for success and failures.

        //! Call sign in endpoint for provider password grant flow.
        //! @param providerName Provider to call sign in.
        //! @param username Username to use to for sign in.
        //! @param password Password to use to for sign in.
        virtual void PasswordGrantSingleFactorSignInAsync(const AZStd::string& providerName, const AZStd::string& username, const AZStd::string& password) = 0;

        //! Call sign in endpoint for provider password grant multi factor authentication flow.
        //! @param providerName Provider to call MFA sign in.
        //! @param username Username to use for MFA sign in.
        //! @param password Password to use for MFA sign in.
        virtual void PasswordGrantMultiFactorSignInAsync(const AZStd::string& providerName, const AZStd::string& username, const AZStd::string& password) = 0;

        //! Call confirm endpoint for provider password grant multi factor authentication flow .
        //! @param providerName Provider to call MFA confirm sign in.
        //! @param username Username to use for MFA confirm.
        //! @param confirmationCode Confirmation code (sent to email/text) to use for MFA confirm.
        virtual void PasswordGrantMultiFactorConfirmSignInAsync(const AZStd::string& providerName, const AZStd::string& username, const AZStd::string& confirmationCode) = 0;

        //! Call code-pair endpoint for provider device grant flow.
        //! @param providerName Provider to call device sign in.
        virtual void DeviceCodeGrantSignInAsync(const AZStd::string& providerName) = 0;

        //! Call tokens endpoint for provider device grant flow.
        //! @param providerName Provider to call device confirm sign in.
        virtual void DeviceCodeGrantConfirmSignInAsync(const AZStd::string& providerName) = 0;

        //! Call refresh endpoint for provider refresh grant flow.
        //! @param providerName Provider to call refresh tokens.
        virtual void RefreshTokensAsync(const AZStd::string& providerName) = 0;

        //! Call refresh token if token not valid. If token valid, fires corresponding event.
        //! @param providerName Provider to get access token for.
        //! events: OnRefreshTokensSuccess, OnRefreshTokensFail
        virtual void GetTokensWithRefreshAsync(const AZStd::string& providerName) = 0;

        //! Signs user out.
        //! Clears all cached tokens.
        //! @param providerName Provider to sign out.
        //! @return bool True: Successfully sign out.
        virtual bool SignOut(const AZStd::string& providerName) = 0;

        //////////////////////////////////////////////////////////////////////////
    };

    //! Authentication Request bus for different supported providers.
    class AuthenticationProviderScriptCanvasRequests
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
    using AuthenticationProviderScriptCanvasRequestBus = AZ::EBus<IAuthenticationProviderScriptCanvasRequests, AuthenticationProviderScriptCanvasRequests>;

} // namespace AWSClientAuth
