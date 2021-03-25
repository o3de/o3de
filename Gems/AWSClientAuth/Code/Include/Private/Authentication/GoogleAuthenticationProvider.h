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
        bool Initialize(AZStd::weak_ptr<AZ::SettingsRegistryInterface> settingsRegistry) override;
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
