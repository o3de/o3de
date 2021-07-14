/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/IO/FileIO.h>

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
        AuthenticationProviderScriptCanvasRequestBus::Handler::BusConnect();
    }

    AuthenticationProviderManager::~AuthenticationProviderManager()
    {
        ResetProviders();
        AuthenticationProviderScriptCanvasRequestBus::Handler::BusDisconnect();
        AuthenticationProviderRequestBus::Handler::BusDisconnect();
        AZ::Interface<IAuthenticationProviderRequests>::Unregister(this);
    }

    bool AuthenticationProviderManager::Initialize(const AZStd::vector<ProviderNameEnum>& providerNames)
    {
        ResetProviders();

        bool initializeSuccess = true;

        for (auto providerName : providerNames)
        {
            m_authenticationProvidersMap[providerName] = CreateAuthenticationProviderObject(providerName);
            initializeSuccess = initializeSuccess && m_authenticationProvidersMap[providerName]->Initialize();
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
                return;
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

    ProviderNameEnum AuthenticationProviderManager::GetProviderNameEnum(AZStd::string name)
    {
        auto enumValue = ProviderNameEnumNamespace::FromStringToProviderNameEnum(name);
        if (enumValue.has_value())
        {
            return enumValue.value();
        }
        AZ_Warning("AuthenticationProviderManager", false, "Incorrect string value for enum: %s", name.c_str());
        return ProviderNameEnum::None;
    }

    bool AuthenticationProviderManager::Initialize(
        const AZStd::vector<AZStd::string>& providerNames)
    {
        AZStd::vector<ProviderNameEnum> providerNamesEnum;
        for (auto name : providerNames)
        {
            providerNamesEnum.push_back(GetProviderNameEnum(name));
        }
        return Initialize(providerNamesEnum);
    }

    void AuthenticationProviderManager::PasswordGrantSingleFactorSignInAsync(const AZStd::string& providerName, const AZStd::string& username, const AZStd::string& password)
    {
        PasswordGrantSingleFactorSignInAsync(GetProviderNameEnum(providerName), username, password);
    }

    void AuthenticationProviderManager::PasswordGrantMultiFactorSignInAsync(const AZStd::string& providerName, const AZStd::string& username, const AZStd::string& password)
    {
        PasswordGrantMultiFactorSignInAsync(GetProviderNameEnum(providerName), username, password);
    }

    void AuthenticationProviderManager::PasswordGrantMultiFactorConfirmSignInAsync(const AZStd::string& providerName, const AZStd::string& username, const AZStd::string& confirmationCode)
    {
        PasswordGrantMultiFactorConfirmSignInAsync(GetProviderNameEnum(providerName), username, confirmationCode);
    }

    void AuthenticationProviderManager::DeviceCodeGrantSignInAsync(const AZStd::string& providerName)
    {
        DeviceCodeGrantSignInAsync(GetProviderNameEnum(providerName));
    }

    void AuthenticationProviderManager::DeviceCodeGrantConfirmSignInAsync(const AZStd::string& providerName)
    {
        DeviceCodeGrantConfirmSignInAsync(GetProviderNameEnum(providerName));
    }

    void AuthenticationProviderManager::RefreshTokensAsync(const AZStd::string& providerName)
    {
        RefreshTokensAsync(GetProviderNameEnum(providerName));
    }

    void AuthenticationProviderManager::GetTokensWithRefreshAsync(const AZStd::string& providerName)
    {
        GetTokensWithRefreshAsync(GetProviderNameEnum(providerName));
    }

    bool AuthenticationProviderManager::IsSignedIn(const AZStd::string& providerName)
    {
        return IsSignedIn(GetProviderNameEnum(providerName));
    }

    bool AuthenticationProviderManager::SignOut(const AZStd::string& providerName)
    {
        return SignOut(GetProviderNameEnum(providerName));
    }

    AuthenticationTokens AuthenticationProviderManager::GetAuthenticationTokens(const AZStd::string& providerName)
    {
        return GetAuthenticationTokens(GetProviderNameEnum(providerName));
    }

} // namespace AWSClientAuth

