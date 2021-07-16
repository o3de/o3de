/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/containers/map.h>
#include <AzCore/std/containers/vector.h>
#include <Authentication/AuthenticationProviderBus.h>
#include <Authentication/AuthenticationProviderScriptCanvasBus.h>
#include <Authentication/AuthenticationProviderInterface.h>
#include <Authentication/AuthenticationTokens.h>

namespace AWSClientAuth
{
     //! Manages various authentication provider implementations and implements AuthenticationProvider Request bus.
    class AuthenticationProviderManager
        : public AuthenticationProviderRequestBus::Handler
        , public AuthenticationProviderScriptCanvasRequestBus::Handler
    {
    public:
        AZ_RTTI(AuthenticationProviderManager, "{45813BA5-9A46-4A2A-A923-C79CFBA0E63D}", IAuthenticationProviderRequests);
        AuthenticationProviderManager();
        virtual ~AuthenticationProviderManager();

    protected:
        // AuthenticationProviderRequestsBus Interface
        bool Initialize(const AZStd::vector<ProviderNameEnum>& providerNames) override;
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
        
        // AuthenticationProviderScriptCanvasRequest interface
        bool Initialize(const AZStd::vector<AZStd::string>& providerNames) override;
        void PasswordGrantSingleFactorSignInAsync(
            const AZStd::string& providerName, const AZStd::string& username, const AZStd::string& password) override;
        void PasswordGrantMultiFactorSignInAsync(
            const AZStd::string& providerName, const AZStd::string& username, const AZStd::string& password) override;
        void PasswordGrantMultiFactorConfirmSignInAsync(
            const AZStd::string& providerName, const AZStd::string& username, const AZStd::string& confirmationCode) override;
        void DeviceCodeGrantSignInAsync(const AZStd::string& providerName) override;
        void DeviceCodeGrantConfirmSignInAsync(const AZStd::string& providerName) override;
        void RefreshTokensAsync(const AZStd::string& providerName) override;
        void GetTokensWithRefreshAsync(const AZStd::string& providerName) override;
        bool IsSignedIn(const AZStd::string& providerName) override;
        bool SignOut(const AZStd::string& providerName) override;
        AuthenticationTokens GetAuthenticationTokens(const AZStd::string& providerName) override;

        virtual AZStd::unique_ptr<AuthenticationProviderInterface> CreateAuthenticationProviderObject(const ProviderNameEnum& providerName);
        AZStd::map<ProviderNameEnum, AZStd::unique_ptr<AuthenticationProviderInterface>> m_authenticationProvidersMap;

    private:
        bool IsProviderInitialized(const ProviderNameEnum& providerName);
        void ResetProviders();
        ProviderNameEnum GetProviderNameEnum(AZStd::string name);
    };

} // namespace AWSClientAuth
