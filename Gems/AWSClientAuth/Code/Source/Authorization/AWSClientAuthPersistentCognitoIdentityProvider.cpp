/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Authorization/AWSClientAuthPersistentCognitoIdentityProvider.h>

namespace AWSClientAuth
{
    AWSClientAuthPersistentCognitoIdentityProvider::~AWSClientAuthPersistentCognitoIdentityProvider()
    {
        m_logins.clear();
        m_awsAccountId = "";
        m_identityPoolId = "";
        m_identityId = "";
        m_identityIdUpdatedCallback = nullptr;
        m_loginsUpdatedCallback = nullptr;
    }

    void AWSClientAuthPersistentCognitoIdentityProvider::Initialize(const Aws::String& awsAccountId, const Aws::String& identityPoolId)
    {
        m_identityPoolId = identityPoolId;
        m_awsAccountId = awsAccountId;
    }

    bool AWSClientAuthPersistentCognitoIdentityProvider::HasIdentityId() const
    {
        return !m_identityId.empty();
    }

    bool AWSClientAuthPersistentCognitoIdentityProvider::HasLogins() const
    {
        return m_logins.size() > 0;
    }

    Aws::String AWSClientAuthPersistentCognitoIdentityProvider::GetIdentityId() const
    {
        return m_identityId;
    }

    Aws::Map<Aws::String, Aws::Auth::LoginAccessTokens> AWSClientAuthPersistentCognitoIdentityProvider::GetLogins()
    {
        return m_logins;
    }

    Aws::String AWSClientAuthPersistentCognitoIdentityProvider::GetAccountId() const
    {
        return m_awsAccountId;
    }

    Aws::String AWSClientAuthPersistentCognitoIdentityProvider::GetIdentityPoolId() const
    {
        return m_identityPoolId;
    }

    void AWSClientAuthPersistentCognitoIdentityProvider::PersistIdentityId(const Aws::String& identityId)
    {
        m_identityId = identityId;
        if (m_identityIdUpdatedCallback)
        {
            m_identityIdUpdatedCallback(*this);
        }
    }

    void AWSClientAuthPersistentCognitoIdentityProvider::PersistLogins(const Aws::Map<Aws::String, Aws::Auth::LoginAccessTokens>& logins)
    {
       m_logins = logins;
       if (m_loginsUpdatedCallback)
       {
           m_loginsUpdatedCallback(*this);
       }       
    }

    void AWSClientAuthPersistentCognitoIdentityProvider::RemoveLogin(const Aws::String& key)
    {
        m_logins.erase(key);
        if (m_loginsUpdatedCallback)
        {
            m_loginsUpdatedCallback(*this);
        }
    }

} // namespace AWSClientAuth
