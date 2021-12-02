/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <aws/cognito-identity/CognitoIdentityClient.h>
#include <aws/identity-management/auth/CognitoCachingCredentialsProvider.h>
#include <aws/identity-management/auth/PersistentCognitoIdentityProvider.h>

namespace AWSClientAuth
{
    //! Cognito Caching Credentials Provider implementation that is derived from AWS Native SDK.
    //! For use with authenticated credentials.
    class AWSClientAuthCognitoCachingAuthenticatedCredentialsProvider
        : public Aws::Auth::CognitoCachingCredentialsProvider
    {
    public:
        AWSClientAuthCognitoCachingAuthenticatedCredentialsProvider(
            const std::shared_ptr<Aws::Auth::PersistentCognitoIdentityProvider>& identityRepository,
            const std::shared_ptr<Aws::CognitoIdentity::CognitoIdentityClient>& cognitoIdentityClient = nullptr);

    protected:
        Aws::CognitoIdentity::Model::GetCredentialsForIdentityOutcome GetCredentialsFromCognito() const override;
    };

    //! Cognito Caching Credentials Provider implementation that is eventually derived from AWS Native SDK.
    //! For use with anonymous credentials.
    class AWSClientAuthCachingAnonymousCredsProvider : public AWSClientAuthCognitoCachingAuthenticatedCredentialsProvider
    {
    public:
        AWSClientAuthCachingAnonymousCredsProvider(
            const std::shared_ptr<Aws::Auth::PersistentCognitoIdentityProvider>& identityRepository,
            const std::shared_ptr<Aws::CognitoIdentity::CognitoIdentityClient>& cognitoIdentityClient = nullptr);

    protected:
        Aws::CognitoIdentity::Model::GetCredentialsForIdentityOutcome GetCredentialsFromCognito() const override;
    };

} // namespace AWSClientAuth
