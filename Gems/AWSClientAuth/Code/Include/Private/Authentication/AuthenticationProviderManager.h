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

#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/containers/map.h>
#include <AzCore/std/containers/vector.h>
#include <Authentication/AuthenticationProviderBus.h>
#include <Authentication/AuthenticationProviderInterface.h>
#include <Authentication/AuthenticationTokens.h>

namespace AWSClientAuth
{
     //! Manages various authentication provider implementations and implements AuthenticationProvider Request bus.
    class AuthenticationProviderManager
        : AuthenticationProviderRequestBus::Handler
    {
    public:
        AZ_RTTI(AuthenticationProviderManager, "{45813BA5-9A46-4A2A-A923-C79CFBA0E63D}", IAuthenticationProviderRequests);
        AuthenticationProviderManager();
        virtual ~AuthenticationProviderManager();

    protected:
        // AuthenticationProviderRequestsBus Interface
        bool Initialize(const AZStd::vector<ProviderNameEnum>& providerNames, const AZStd::string& settingsRegistryPath) override;
        void PasswordGrantSingleFactorSignInAsync(const ProviderNameEnum& providerName, const AZStd::string& username, const AZStd::string& password) override;
        void PasswordGrantMultiFactorSignInAsync(const ProviderNameEnum& providerName, const AZStd::string& username, const AZStd::string& password) override;
        void PasswordGrantMultiFactorConfirmSignInAsync(const ProviderNameEnum& providerName, const AZStd::string& username, const AZStd::string& confirmationCode) override;
        void DeviceCodeGrantSignInAsync(const ProviderNameEnum& providerName) override;
        void DeviceCodeGrantConfirmSignInAsync(const ProviderNameEnum& providerName) override;
        void RefreshTokensAsync(const ProviderNameEnum& providerName) override;
        void GetTokensWithRefreshAsync(const ProviderNameEnum& providerName) override;
        bool IsSignedIn(const ProviderNameEnum& providerName) override;
        bool SignOut(const ProviderNameEnum& providerName) override;
        AuthenticationTokens GetAuthenticationTokens(const ProviderNameEnum& providerName) override;

        virtual AZStd::unique_ptr<AuthenticationProviderInterface> CreateAuthenticationProviderObject(const ProviderNameEnum& providerName);
        AZStd::map<ProviderNameEnum, AZStd::unique_ptr<AuthenticationProviderInterface>> m_authenticationProvidersMap;

    private:
        bool IsProviderInitialized(const ProviderNameEnum& providerName);
        void ResetProviders();

        AZStd::shared_ptr<AZ::SettingsRegistryInterface> m_settingsRegistry;

    };

} // namespace AWSClientAuth
