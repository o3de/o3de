/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Settings/SettingsRegistry.h>
#include <AzCore/std/smart_ptr/weak_ptr.h>
#include <Authentication/AuthenticationTokens.h>

namespace AWSClientAuth
{

    //! Interface to be implemented by AuthenticationProviders to interact with AuthenticationManager.
    //! Follows grant types for password and device from following: https://oauth.net/2/grant-types/
    class AuthenticationProviderInterface
    {
    public:
        AuthenticationProviderInterface() = default;
        virtual ~AuthenticationProviderInterface() = default;
      
        //! Extract required settings for the provider from setting registry.
        //! @return bool True: if provider can parse required settings and validate. False: fails to parse required settings.
        virtual bool Initialize() = 0;

        //! Call sign in endpoint for provider password grant flow.
        //! @param username Username to use to for sign in.
        //! @param password Password to use to for sign in.
        virtual void PasswordGrantSingleFactorSignInAsync(const AZStd::string& username, const AZStd::string& password) = 0;

        //! Call sign in endpoint for provider password grant multi factor authentication flow.
        //! @param username Username to use for MFA sign in.
        //! @param password Password to use for MFA sign in.
        virtual void PasswordGrantMultiFactorSignInAsync(const AZStd::string& username, const AZStd::string& password) = 0;

        //! Call confirm endpoint for provider password grant multi factor authentication flow .
        //! @param username Username to use for MFA confirm.
        //! @param confirmationCode Confirmation code (sent to email/text) to use for MFA confirm.
        virtual void PasswordGrantMultiFactorConfirmSignInAsync(const AZStd::string& username, const AZStd::string& confirmationCode) = 0;

        //! Call code-pair endpoint for provider device grant flow.
        virtual void DeviceCodeGrantSignInAsync() = 0;

        //! Call tokens endpoint for provider device grant flow.
        virtual void DeviceCodeGrantConfirmSignInAsync() = 0;

        //! Call refresh endpoint for provider refresh grant flow.
        virtual void RefreshTokensAsync() = 0;

        //! @return Authentication tokens from last successful sign in.
        virtual AuthenticationTokens GetAuthenticationTokens();

        //! Clears all cached tokens and expiry
        virtual void SignOut();

    protected:
        AuthenticationTokens m_authenticationTokens;
    };
} // namespace AWSClientAuth
