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

#include <aws/identity-management/auth/PersistentCognitoIdentityProvider.h>

namespace AWSClientAuth
{
    //! Persistent Cognito Identity provider implementation that is shared with AWS Native SDK client.
    //! Use std::shared_ptr to create instance.
    class AWSClientAuthPersistentCognitoIdentityProvider
        : public Aws::Auth::PersistentCognitoIdentityProvider
    {
    public:
        AWSClientAuthPersistentCognitoIdentityProvider() = default;
        virtual ~AWSClientAuthPersistentCognitoIdentityProvider();
        // PersistentCognitoIdentityProvider Interface
        void Initialize(const Aws::String& awsAccountId, const Aws::String& identityPoolId);
        bool HasIdentityId() const override;
        bool HasLogins() const override;
        Aws::String GetIdentityId() const override;
        Aws::Map<Aws::String, Aws::Auth::LoginAccessTokens> GetLogins() override;
        Aws::String GetAccountId() const override;
        Aws::String GetIdentityPoolId() const override;
        void PersistIdentityId(const Aws::String&) override;
        void PersistLogins(const Aws::Map<Aws::String, Aws::Auth::LoginAccessTokens>&) override;

        void RemoveLogin(const Aws::String&);

    private:
        Aws::Map<Aws::String, Aws::Auth::LoginAccessTokens> m_logins;
        Aws::String m_identityId;
        Aws::String m_awsAccountId;
        Aws::String m_identityPoolId;
    };

} // namespace AWSClientAuth
