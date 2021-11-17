/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Authorization/AWSClientAuthCognitoCachingAuthenticatedCredentialsProvider.h>

#include <AzCore/Debug/Trace.h>

#include <aws/cognito-identity/CognitoIdentityClient.h>
#include <aws/cognito-identity/model/GetCredentialsForIdentityRequest.h>
#include <aws/cognito-identity/model/GetIdRequest.h>
#include <aws/core/utils/Outcome.h>
#include <aws/core/utils/logging/LogMacros.h>
#include <aws/identity-management/auth/CognitoCachingCredentialsProvider.h>
#include <aws/identity-management/auth/PersistentCognitoIdentityProvider.h>


namespace AWSClientAuth
{
    static const char* AUTH_LOG_TAG = "AWSClientAuthCognitoCachingAuthenticatedCredentialsProvider";
    static const char* ANON_LOG_TAG = "AWSClientAuthCachingAnonymousCredsProvider";

    // Modification of https://github.com/aws/aws-sdk-cpp/blob/main/aws-cpp-sdk-identity-management/source/auth/CognitoCachingCredentialsProvider.cpp#L92
    // to work around account ID requirement. Account id is not required for call to succeed and is not set unless provided.
    // see: https://github.com/aws/aws-sdk-cpp/issues/1448
    Aws::CognitoIdentity::Model::GetCredentialsForIdentityOutcome FetchCredsFromCognito(
        const Aws::CognitoIdentity::CognitoIdentityClient& cognitoIdentityClient,
        Aws::Auth::PersistentCognitoIdentityProvider& identityRepository,
        const char* logTag,
        bool includeLogins)
    {
        auto logins = identityRepository.GetLogins();
        Aws::Map<Aws::String, Aws::String> cognitoLogins;
        for (auto& login : logins)
        {
            cognitoLogins[login.first] = login.second.accessToken;
        }

        if (!identityRepository.HasIdentityId())
        {
            auto accountId = identityRepository.GetAccountId();
            auto identityPoolId = identityRepository.GetIdentityPoolId();

            Aws::CognitoIdentity::Model::GetIdRequest getIdRequest;
            getIdRequest.SetIdentityPoolId(identityPoolId);

            if (!accountId.empty()) // new check
            {
                getIdRequest.SetAccountId(accountId);
                AWS_LOGSTREAM_INFO(logTag, "Identity not found, requesting an id for accountId "
                    << accountId << " identity pool id "
                    << identityPoolId << " with logins.");
            }
            else
            {
                AWS_LOGSTREAM_INFO(
                    logTag, "Identity not found, requesting an id for identity pool id %s" << identityPoolId << " with logins.");
            }
            if (includeLogins)
            {
                getIdRequest.SetLogins(cognitoLogins);
            }

            auto getIdOutcome = cognitoIdentityClient.GetId(getIdRequest);
            if (getIdOutcome.IsSuccess())
            {
                auto identityId = getIdOutcome.GetResult().GetIdentityId();
                AWS_LOGSTREAM_INFO(logTag, "Successfully retrieved identity: " << identityId);
                identityRepository.PersistIdentityId(identityId);
            }
            else
            {
                AWS_LOGSTREAM_ERROR(
                    logTag,
                    "Failed to retrieve identity. Error: " << getIdOutcome.GetError().GetExceptionName() << " "
                                                           << getIdOutcome.GetError().GetMessage());
                return Aws::CognitoIdentity::Model::GetCredentialsForIdentityOutcome(getIdOutcome.GetError());
            }
        }

        Aws::CognitoIdentity::Model::GetCredentialsForIdentityRequest getCredentialsForIdentityRequest;
        getCredentialsForIdentityRequest.SetIdentityId(identityRepository.GetIdentityId());
        if (includeLogins)
        {
            getCredentialsForIdentityRequest.SetLogins(cognitoLogins);
        }

        return cognitoIdentityClient.GetCredentialsForIdentity(getCredentialsForIdentityRequest);
    }

    AWSClientAuthCognitoCachingAuthenticatedCredentialsProvider::AWSClientAuthCognitoCachingAuthenticatedCredentialsProvider(
        const std::shared_ptr<Aws::Auth::PersistentCognitoIdentityProvider>& identityRepository,
        const std::shared_ptr<Aws::CognitoIdentity::CognitoIdentityClient>& cognitoIdentityClient)
        : CognitoCachingCredentialsProvider(identityRepository, cognitoIdentityClient)
    {
    }

    Aws::CognitoIdentity::Model::GetCredentialsForIdentityOutcome
    AWSClientAuthCognitoCachingAuthenticatedCredentialsProvider::GetCredentialsFromCognito() const
    {
        return FetchCredsFromCognito(*m_cognitoIdentityClient, *m_identityRepository, AUTH_LOG_TAG, true);
    }

    AWSClientAuthCachingAnonymousCredsProvider::AWSClientAuthCachingAnonymousCredsProvider(
        const std::shared_ptr<Aws::Auth::PersistentCognitoIdentityProvider>& identityRepository,
        const std::shared_ptr<Aws::CognitoIdentity::CognitoIdentityClient>& cognitoIdentityClient)
        : AWSClientAuthCognitoCachingAuthenticatedCredentialsProvider(identityRepository, cognitoIdentityClient)
    {
    }

    Aws::CognitoIdentity::Model::GetCredentialsForIdentityOutcome AWSClientAuthCachingAnonymousCredsProvider::
        GetCredentialsFromCognito() const
    {
        return FetchCredsFromCognito(*m_cognitoIdentityClient, *m_identityRepository, ANON_LOG_TAG, false);
    }


} // namespace AWSClientAuth
