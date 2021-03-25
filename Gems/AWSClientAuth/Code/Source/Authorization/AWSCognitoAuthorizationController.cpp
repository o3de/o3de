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

#include <AWSClientAuthBus.h>
#include <AWSCoreBus.h>
#include <Authorization/AWSCognitoAuthorizationController.h>
#include <AzCore/EBus/Internal/BusContainer.h>
#include <AzCore/Jobs/JobFunction.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/Settings/SettingsRegistryImpl.h>

#include <aws/identity-management/auth/CognitoCachingCredentialsProvider.h>

namespace AWSClientAuth
{
    constexpr char COGNITO_AUTHORIZATION_SETTINGS_PATH[] = "/AWS/CognitoIdentityPool";

    AWSCognitoAuthorizationController::AWSCognitoAuthorizationController()
    {
        AZ::Interface<IAWSCognitoAuthorizationRequests>::Register(this);
        AWSCognitoAuthorizationRequestBus::Handler::BusConnect();
        AuthenticationProviderNotificationBus::Handler::BusConnect();
        AWSCore::AWSCredentialRequestBus::Handler::BusConnect();

        m_settings = AZStd::make_unique<CognitoAuthorizationSettings>();

        m_persistentCognitoIdentityProvider = std::make_shared<AWSClientAuthPersistentCognitoIdentityProvider>();
        m_persistentAnonymousCognitoIdentityProvider = std::make_shared<AWSClientAuthPersistentCognitoIdentityProvider>();

        auto identityClient = AZ::Interface<IAWSClientAuthRequests>::Get()->GetCognitoIdentityClient();

        m_cognitoCachingCredentialsProvider =
            std::make_shared<Aws::Auth::CognitoCachingAuthenticatedCredentialsProvider>(m_persistentCognitoIdentityProvider, identityClient);

        m_cognitoCachingAnonymousCredentialsProvider =
            std::make_shared<Aws::Auth::CognitoCachingAnonymousCredentialsProvider>(m_persistentAnonymousCognitoIdentityProvider, identityClient);
    }

    AWSCognitoAuthorizationController::~AWSCognitoAuthorizationController()
    {
        m_cognitoCachingCredentialsProvider.reset();
        m_persistentAnonymousCognitoIdentityProvider.reset();
        m_persistentCognitoIdentityProvider.reset();
        m_persistentAnonymousCognitoIdentityProvider.reset();

        m_settings.reset();

        AWSCore::AWSCredentialRequestBus::Handler::BusDisconnect();
        AuthenticationProviderNotificationBus::Handler::BusDisconnect();
        AWSCognitoAuthorizationRequestBus::Handler::BusDisconnect();
        AZ::Interface<IAWSCognitoAuthorizationRequests>::Unregister(this);
    }

    bool AWSCognitoAuthorizationController::Initialize(const AZStd::string& settingsRegistryPath)
    {
        AZStd::unique_ptr<AZ::SettingsRegistryInterface> settingsRegistry = AZStd::make_unique<AZ::SettingsRegistryImpl>();

        if (!settingsRegistry->MergeSettingsFile(settingsRegistryPath, AZ::SettingsRegistryInterface::Format::JsonMergePatch))
        {
            AZ_Error("AWSCognitoAuthorizationController", true, "Failed to merge settings file for path %s", settingsRegistryPath.c_str());
            return false;
        }

        if (!settingsRegistry->GetObject(m_settings.get(), azrtti_typeid(m_settings.get()), COGNITO_AUTHORIZATION_SETTINGS_PATH))
        {
            AZ_Error("AWSCognitoAuthorizationController", true, "Failed to get settings object for path %s", COGNITO_AUTHORIZATION_SETTINGS_PATH);
            return false;
        }

        m_persistentCognitoIdentityProvider->Initialize(m_settings->m_awsAccountId.c_str(), m_settings->m_cognitoIdentityPoolId.c_str());
        m_persistentAnonymousCognitoIdentityProvider->Initialize(m_settings->m_awsAccountId.c_str(), m_settings->m_cognitoIdentityPoolId.c_str());

        return true;
    }

    void AWSCognitoAuthorizationController::Reset()
    {
        // Brackets for lock guard scopes
        {
            AZStd::lock_guard<AZStd::mutex> lock(m_persistentAnonymousCognitoIdentityProviderMutex);
            m_persistentAnonymousCognitoIdentityProvider->ClearLogins();
            m_persistentAnonymousCognitoIdentityProvider->ClearIdentity();
        }

        {
            AZStd::lock_guard<AZStd::mutex> lock(m_persistentCognitoIdentityProviderMutex);
            m_persistentCognitoIdentityProvider->ClearLogins();
            m_persistentCognitoIdentityProvider->ClearIdentity();
        }
    }

    AZStd::string AWSCognitoAuthorizationController::GetIdentityId()
    {   
        // Give preference to authenticated credentials provider.
        if (HasPersistedLogins())
        {
            AZStd::lock_guard<AZStd::mutex> lock(m_persistentCognitoIdentityProviderMutex);
            return m_persistentCognitoIdentityProvider->GetIdentityId().c_str();
        }
        else
        {
            AZStd::lock_guard<AZStd::mutex> lock(m_persistentAnonymousCognitoIdentityProviderMutex);
            return m_persistentAnonymousCognitoIdentityProvider->GetIdentityId().c_str();
        }
    }

    bool AWSCognitoAuthorizationController::HasPersistedLogins()
    {
        AZStd::lock_guard<AZStd::mutex> lock(m_persistentCognitoIdentityProviderMutex);
        return m_persistentCognitoIdentityProvider->HasLogins();
    }

    std::shared_ptr<Aws::Auth::AWSCredentialsProvider> AWSCognitoAuthorizationController::GetCognitoCredentialsProvider()
    {
        return m_cognitoCachingCredentialsProvider;
    }

    std::shared_ptr<Aws::Auth::AWSCredentialsProvider> AWSCognitoAuthorizationController::GetAnonymousCognitoCredentialsProvider()
    {
        return m_cognitoCachingAnonymousCredentialsProvider;
    }

    void AWSCognitoAuthorizationController::RequestAWSCredentialsAsync()
    {
        bool anonymous = true;
        // Give preference to authenticated credentials provider.
        if (m_persistentCognitoIdentityProvider->HasLogins())
        {
            anonymous = false;    
        }
        else
        {
            AZ_Warning("AWSCognitoAuthorizationController", true, "No logins found. Fetching anonymous/unauthenticated credentials");
        }

        AZ::JobContext* jobContext = nullptr;
        AWSCore::AWSCoreRequestBus::BroadcastResult(jobContext, &AWSCore::AWSCoreRequests::GetDefaultJobContext);
        AZ::Job* job = AZ::CreateJobFunction(
            [this, anonymous]() {
                Aws::Auth::AWSCredentials credentials;
                // GetAWSCredentials makes Cognito GetId and GetCredentialsForIdentity Cognito identity pool API request if no valid cached credentials found.
                if (anonymous)
                {
                    AZStd::lock_guard<AZStd::mutex> lock(m_persistentAnonymousCognitoIdentityProviderMutex);
                    credentials = m_cognitoCachingAnonymousCredentialsProvider->GetAWSCredentials();
                }
                else
                {
                    AZStd::lock_guard<AZStd::mutex> lock(m_persistentCognitoIdentityProviderMutex);
                    credentials = m_cognitoCachingCredentialsProvider->GetAWSCredentials();
                }
                
                if (!credentials.IsEmpty())
                {
                    ClientAuthAWSCredentials clientAuthAWSCrendentials(credentials.GetAWSAccessKeyId().c_str(), credentials.GetAWSSecretKey().c_str(), credentials.GetSessionToken().c_str());
                    AWSClientAuth::AWSCognitoAuthorizationNotificationBus::Broadcast(
                        &AWSClientAuth::AWSCognitoAuthorizationNotifications::OnRequestAWSCredentialsSuccess, clientAuthAWSCrendentials);
                }
                else
                {
                    AWSClientAuth::AWSCognitoAuthorizationNotificationBus::Broadcast(
                        &AWSClientAuth::AWSCognitoAuthorizationNotifications::OnRequestAWSCredentialsFail,
                        "Failed to get AWS credentials");
                }
            },
            true, jobContext);
        job->Start();
    }

    AZStd::string AWSCognitoAuthorizationController::GetAuthenticationProviderId(const ProviderNameEnum& providerName)
    {
        switch (providerName)
        {
        case ProviderNameEnum::AWSCognitoIDP:
        {
            return m_settings->m_cognitoUserPoolId;
        }
        case ProviderNameEnum::LoginWithAmazon:
        {
            return m_settings->m_loginWithAmazonId;
        }
        case ProviderNameEnum::Google:
        {
            return m_settings->m_googleId;
        }
        default:
        {
            return "";
        }
        }
    }

    void AWSCognitoAuthorizationController::PersistLoginsAndRefreshAWSCredentials(const AuthenticationTokens& authenticationTokens)
    {
        // lock to persist logins as the object is shared with Native SDK. Native SDK reads logins and persists identity id and expiry.
        AZStd::lock_guard<AZStd::mutex> lock(m_persistentCognitoIdentityProviderMutex);

        // Save logins to the shared persistent Cognito identity provider for authenticated authorization.
        // Append logins to existing map.
        Aws::Map<Aws::String, Aws::Auth::LoginAccessTokens> logins = m_persistentCognitoIdentityProvider->GetLogins();
        Aws::Auth::LoginAccessTokens tokens;
        tokens.accessToken = authenticationTokens.GetOpenIdToken().c_str();

        logins[GetAuthenticationProviderId(authenticationTokens.GetProviderName()).c_str()] = tokens;
        m_persistentCognitoIdentityProvider->PersistLogins(logins);
    }

    void AWSCognitoAuthorizationController::OnPasswordGrantSingleFactorSignInSuccess(const AWSClientAuth::AuthenticationTokens& authenticationTokens)
    {
        PersistLoginsAndRefreshAWSCredentials(authenticationTokens);
    }

    void AWSCognitoAuthorizationController::OnPasswordGrantMultiFactorConfirmSignInSuccess(
        const AWSClientAuth::AuthenticationTokens& authenticationTokens)
    {
        PersistLoginsAndRefreshAWSCredentials(authenticationTokens);
    }

    void AWSCognitoAuthorizationController::OnDeviceCodeGrantConfirmSignInSuccess(
        const AWSClientAuth::AuthenticationTokens& authenticationTokens)
    {
        PersistLoginsAndRefreshAWSCredentials(authenticationTokens);
    }

    void AWSCognitoAuthorizationController::OnRefreshTokensSuccess(const AWSClientAuth::AuthenticationTokens& authenticationTokens)
    {
        PersistLoginsAndRefreshAWSCredentials(authenticationTokens);
    }

    void AWSCognitoAuthorizationController::OnSignOut(const ProviderNameEnum& provideName)
    {
        // lock to persist logins as the object is shared with Native SDK.
        AZStd::lock_guard<AZStd::mutex> lock(m_persistentCognitoIdentityProviderMutex);
        m_persistentCognitoIdentityProvider->RemoveLogin(GetAuthenticationProviderId(provideName).c_str());
    }

    int AWSCognitoAuthorizationController::GetCredentialHandlerOrder() const
    {
        return AWSCore::CredentialHandlerOrder::COGNITO_IDENITY_POOL_CREDENTIAL_HANDLER;
    }

    std::shared_ptr<Aws::Auth::AWSCredentialsProvider> AWSCognitoAuthorizationController::GetCredentialsProvider()
    {
        // If logins are persisted default to using authenticated credentials provide.
        // Check authenticated credentials to verify persisted logins are valid.
        if (HasPersistedLogins())
        {
            // lock to protect logins being persisted.
            AZStd::lock_guard<AZStd::mutex> lock(m_persistentCognitoIdentityProviderMutex);
            if (!m_cognitoCachingCredentialsProvider->GetAWSCredentials().IsEmpty())
            {
                return m_cognitoCachingCredentialsProvider;
            }
        }

        // lock to protect getting identity id.
        AZStd::lock_guard<AZStd::mutex> lock(m_persistentAnonymousCognitoIdentityProviderMutex);
        // Check anonymous credentials as they are optional settings in Cognito Identity pool.
        if (!m_cognitoCachingAnonymousCredentialsProvider->GetAWSCredentials().IsEmpty())
        {
            AZ_Warning("AWSCognitoAuthorizationCredentialHandler", true, "No logins found. Using Anonymous credential provider");
            return m_cognitoCachingAnonymousCredentialsProvider;
        }

        return nullptr;
    }
 } // namespace AWSClientAuth
