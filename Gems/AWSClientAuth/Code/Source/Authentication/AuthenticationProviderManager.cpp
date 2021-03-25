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

#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/Settings/SettingsRegistryImpl.h>

#include <Authentication/AuthenticationProviderTypes.h>
#include <Authentication/AWSCognitoAuthenticationProvider.h>
#include <Authentication/LWAAuthenticationProvider.h>
#include <Authentication/GoogleAuthenticationProvider.h>
#include <Authentication/AuthenticationProviderManager.h>

namespace AWSClientAuth
{
    AuthenticationProviderManager::AuthenticationProviderManager()
    {
        AZ::Interface<IAuthenticationProviderRequests>::Register(this);
        AuthenticationProviderRequestBus::Handler::BusConnect();
    }

    AuthenticationProviderManager::~AuthenticationProviderManager()
    {
        ResetProviders();
        m_settingsRegistry.reset();
        AuthenticationProviderRequestBus::Handler::BusDisconnect();
        AZ::Interface<IAuthenticationProviderRequests>::Unregister(this);
    }

    bool AuthenticationProviderManager::Initialize(const AZStd::vector<ProviderNameEnum>& providerNames, const AZStd::string& settingsRegistryPath)
    {
        ResetProviders();
        m_settingsRegistry.reset();
        m_settingsRegistry = AZStd::make_shared<AZ::SettingsRegistryImpl>();

        if (!m_settingsRegistry->MergeSettingsFile(settingsRegistryPath, AZ::SettingsRegistryInterface::Format::JsonMergePatch))
        {
            AZ_Error("AuthenticationProviderManager", true, "Error merging settings registry for path: %s", settingsRegistryPath.c_str());
            return false;
        }

        bool initializeSuccess = true;

        for (auto providerName : providerNames)
        {
            m_authenticationProvidersMap[providerName] = CreateAuthenticationProviderObject(providerName);
            initializeSuccess = initializeSuccess && m_authenticationProvidersMap[providerName]->Initialize(m_settingsRegistry);
        }

        return initializeSuccess;
    }

    void AuthenticationProviderManager::PasswordGrantSingleFactorSignInAsync(const ProviderNameEnum& providerName, const AZStd::string& username, const AZStd::string& password)
    {
        if (IsProviderInitialized(providerName))
        {
            m_authenticationProvidersMap[providerName]->PasswordGrantSingleFactorSignInAsync(username, password);
        }
    }

    void AuthenticationProviderManager::PasswordGrantMultiFactorSignInAsync(const ProviderNameEnum& providerName, const AZStd::string& username, const AZStd::string& password)
    {
        if (IsProviderInitialized(providerName))
        {
            m_authenticationProvidersMap[providerName]->PasswordGrantMultiFactorSignInAsync(username, password);
        }
    }

    void AuthenticationProviderManager::PasswordGrantMultiFactorConfirmSignInAsync(const ProviderNameEnum& providerName, const AZStd::string& username, const AZStd::string& confirmationCode)
    {
        if (IsProviderInitialized(providerName))
        {
            m_authenticationProvidersMap[providerName]->PasswordGrantMultiFactorConfirmSignInAsync(username, confirmationCode);
        }
    }

    void AuthenticationProviderManager::DeviceCodeGrantSignInAsync(const ProviderNameEnum& providerName)
    {
        if (IsProviderInitialized(providerName))
        {
            m_authenticationProvidersMap[providerName]->DeviceCodeGrantSignInAsync();
        }
    }

    void AuthenticationProviderManager::DeviceCodeGrantConfirmSignInAsync(const ProviderNameEnum& providerName)
    {
        if (IsProviderInitialized(providerName))
        {
            m_authenticationProvidersMap[providerName]->DeviceCodeGrantConfirmSignInAsync();
        }
    }

    void AuthenticationProviderManager::RefreshTokensAsync(const ProviderNameEnum& providerName)
    {
        if (IsProviderInitialized(providerName))
        {
            m_authenticationProvidersMap[providerName]->RefreshTokensAsync();
        }
    }

    void AuthenticationProviderManager::GetTokensWithRefreshAsync(const ProviderNameEnum& providerName)
    {
        if (!IsProviderInitialized(providerName))
        {
            AuthenticationProviderNotificationBus::Broadcast(&AuthenticationProviderNotifications::OnRefreshTokensFail
                , "Provider is not initialized");
        }

        AuthenticationTokens tokens = m_authenticationProvidersMap[providerName]->GetAuthenticationTokens();
        if (tokens.AreTokensValid())
        {
            AuthenticationProviderNotificationBus::Broadcast(&AuthenticationProviderNotifications::OnRefreshTokensSuccess, tokens);
        }
        else
        {
            m_authenticationProvidersMap[providerName]->RefreshTokensAsync();
        }
    }

    bool AuthenticationProviderManager::IsSignedIn(const ProviderNameEnum& providerName)
    {
        if (IsProviderInitialized(providerName))
        {
            return m_authenticationProvidersMap[providerName]->GetAuthenticationTokens().AreTokensValid();
        }
        return false;
    }

    bool AuthenticationProviderManager::SignOut(const ProviderNameEnum& providerName)
    {
        if (IsProviderInitialized(providerName))
        {
            m_authenticationProvidersMap[providerName]->SignOut();
            AuthenticationProviderNotificationBus::Broadcast(&AuthenticationProviderNotifications::OnSignOut, providerName);
            return true;
        }
        return false;
    }

    AuthenticationTokens AuthenticationProviderManager::GetAuthenticationTokens(const ProviderNameEnum& providerName)
    {
        return m_authenticationProvidersMap[providerName]->GetAuthenticationTokens();
    }

    AZStd::unique_ptr<AuthenticationProviderInterface> AuthenticationProviderManager::CreateAuthenticationProviderObject(const ProviderNameEnum& providerName)
    {
        switch (providerName)
        {
        case ProviderNameEnum::AWSCognitoIDP:
            return AZStd::make_unique<AWSCognitoAuthenticationProvider>();
        case ProviderNameEnum::LoginWithAmazon:
            return AZStd::make_unique<LWAAuthenticationProvider>();
        case ProviderNameEnum::Google:
            return AZStd::make_unique<GoogleAuthenticationProvider>();
        default:
            return nullptr;

        }
    }

    bool AuthenticationProviderManager::IsProviderInitialized(const ProviderNameEnum& providerName)
    {
        bool ret = m_authenticationProvidersMap.contains(providerName);
        AZ_Assert(ret, "ProviderName enum %i not initialized. Please call initialize first");
        return ret;
    }

    void AuthenticationProviderManager::ResetProviders()
    {
        for (auto& [providerName, providerInterface] : m_authenticationProvidersMap)
        {
            providerInterface.reset();
        }
    }

} // namespace AWSClientAuth

