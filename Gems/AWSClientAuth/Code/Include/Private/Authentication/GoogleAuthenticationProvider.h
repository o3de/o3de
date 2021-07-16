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
#include <aws/core/utils/json/JsonSerializer.h>

namespace AWSClientAuth
{
    //! Implements OAuth2.0 device flow for Google authentication service.
    class GoogleAuthenticationProvider
        : public AuthenticationProviderInterface
    {
    public:
        GoogleAuthenticationProvider();
        virtual ~GoogleAuthenticationProvider();

        // AuthenticationProviderInterface overrides
        bool Initialize() override;
        void PasswordGrantSingleFactorSignInAsync(const AZStd::string& username, const AZStd::string& password) override;
        void PasswordGrantMultiFactorSignInAsync(const AZStd::string& username, const AZStd::string& password) override;
        void PasswordGrantMultiFactorConfirmSignInAsync(const AZStd::string& username, const AZStd::string& confirmationCode) override;
        void DeviceCodeGrantSignInAsync() override;
        void DeviceCodeGrantConfirmSignInAsync() override;
        void RefreshTokensAsync() override;

    private:
        void UpdateTokens(const Aws::Utils::Json::JsonView& jsonView);

    protected:
        AZStd::unique_ptr<GoogleProviderSetting> m_settings;

    private:
        AZStd::string m_cachedDeviceCode;
    };
} // namespace AWSClientAuth
