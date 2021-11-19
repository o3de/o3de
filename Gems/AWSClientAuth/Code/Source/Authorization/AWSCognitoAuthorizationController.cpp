/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AWSClientAuthBus.h>
#include <AWSCoreBus.h>
#include <Authorization/AWSClientAuthCognitoCachingAuthenticatedCredentialsProvider.h>
#include <Authorization/AWSCognitoAuthorizationController.h>
#include <ResourceMapping/AWSResourceMappingBus.h>
#include <AWSClientAuthResourceMappingConstants.h>

#include <AzCore/EBus/Internal/BusContainer.h>
#include <AzCore/Jobs/JobFunction.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/Settings/SettingsRegistryImpl.h>

#include <aws/identity-management/auth/CognitoCachingCredentialsProvider.h>

namespace AWSClientAuth
{
    constexpr char CognitoAmazonLoginsId[] = "www.amazon.com";
    constexpr char CognitoGoogleLoginsId[] = "accounts.google.com";
    constexpr char CognitoUserPoolIdFormat[] = "cognito-idp.%s.amazonaws.com/%s";

    AWSCognitoAuthorizationController::AWSCognitoAuthorizationController()
    {
        AZ::Interface<IAWSCognitoAuthorizationRequests>::Register(this);
        AWSCognitoAuthorizationRequestBus::Handler::BusConnect();
        AuthenticationProviderNotificationBus::Handler::BusConnect();
        AWSCore::AWSCredentialRequestBus::Handler::BusConnect();

        m_persistentCognitoIdentityProvider = std::make_shared<AWSClientAuthPersistentCognitoIdentityProvider>();
        m_persistentAnonymousCognitoIdentityProvider = std::make_shared<AWSClientAuthPersistentCognitoIdentityProvider>();

        auto identityClient = AZ::Interface<IAWSClientAuthRequests>::Get()->GetCognitoIdentityClient();

        m_cognitoCachingCredentialsProvider =
            std::make_shared<AWSClientAuthCognitoCachingAuthenticatedCredentialsProvider>(
            m_persistentCognitoIdentityProvider, identityClient);

        m_cognitoCachingAnonymousCredentialsProvider =
            std::make_shared<AWSClientAuthCachingAnonymousCredsProvider>(
            m_persistentAnonymousCognitoIdentityProvider, identityClient);
    }

    AWSCognitoAuthorizationController::~AWSCognitoAuthorizationController()
    {
        m_cognitoCachingCredentialsProvider.reset();
        m_persistentAnonymousCognitoIdentityProvider.reset();
        m_persistentCognitoIdentityProvider.reset();
        m_persistentAnonymousCognitoIdentityProvider.reset();

        AWSCore::AWSCredentialRequestBus::Handler::BusDisconnect();
        AuthenticationProviderNotificationBus::Handler::BusDisconnect();
        AWSCognitoAuthorizationRequestBus::Handler::BusDisconnect();
        AZ::Interface<IAWSCognitoAuthorizationRequests>::Unregister(this);
    }

    bool AWSCognitoAuthorizationController::Initialize()
    {
        AWSCore::AWSResourceMappingRequestBus::BroadcastResult(
            m_awsAccountId, &AWSCore::AWSResourceMappingRequests::GetDefaultAccountId);

        AWSCore::AWSResourceMappingRequestBus::BroadcastResult(
            m_cognitoIdentityPoolId, &AWSCore::AWSResourceMappingRequests::GetResourceNameId, CognitoIdentityPoolIdResourceMappingKey);

        if (m_awsAccountId.empty())
        {
            AZ_TracePrintf("AWSCognitoAuthorizationController", "AWS account id not not configured. Proceeding without it.");
        }

        if (m_cognitoIdentityPoolId.empty())
        {
            AZ_Warning("AWSCognitoAuthorizationController", !m_cognitoIdentityPoolId.empty(), "Missing Cognito Identity pool id in resource mappings.");
            return false;
        }

        AZStd::string userPoolId;
        AWSCore::AWSResourceMappingRequestBus::BroadcastResult(
            userPoolId, &AWSCore::AWSResourceMappingRequests::GetResourceNameId, CognitoUserPoolIdResourceMappingKey);
        AZ_Warning("AWSCognitoAuthorizationController", !userPoolId.empty(), "Missing Cognito User pool id in resource mappings. Cognito IDP authenticated identities will no work.");

        AZStd::string defaultRegion;
        AWSCore::AWSResourceMappingRequestBus::BroadcastResult(
            defaultRegion, &AWSCore::AWSResourceMappingRequests::GetDefaultRegion);
        m_formattedCognitoUserPoolId = AZStd::string::format(CognitoUserPoolIdFormat, defaultRegion.c_str(), userPoolId.c_str());

        m_persistentCognitoIdentityProvider->Initialize(m_awsAccountId.c_str(), m_cognitoIdentityPoolId.c_str());
        m_persistentAnonymousCognitoIdentityProvider->Initialize(m_awsAccountId.c_str(), m_cognitoIdentityPoolId.c_str());

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
            AZ_Warning("AWSCognitoAuthorizationController", false, "No logins found. Fetching anonymous/unauthenticated credentials");
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
            return m_formattedCognitoUserPoolId;
        }
        case ProviderNameEnum::LoginWithAmazon:
        {
            return CognitoAmazonLoginsId;
        }
        case ProviderNameEnum::Google:
        {
            return CognitoGoogleLoginsId;
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
            AZ_Warning("AWSCognitoAuthorizationCredentialHandler", false, "No logins found. Using Anonymous credential provider");
            return m_cognitoCachingAnonymousCredentialsProvider;
        }

        return nullptr;
    }
 } // namespace AWSClientAuth
