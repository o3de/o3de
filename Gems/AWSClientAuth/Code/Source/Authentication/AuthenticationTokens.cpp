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

#include <Authentication/AuthenticationTokens.h>

namespace AWSClientAuth
{
    //! Used to share authentication tokens to caller and to AWSCognitoAuthorizationController.
   
    AuthenticationTokens::AuthenticationTokens()
    {
        m_tokensExpireTimeStamp = AZStd::chrono::system_clock::time_point::min();
        m_providerName = ProviderNameEnum::None;
    }

    AuthenticationTokens::AuthenticationTokens(const AuthenticationTokens& other)
    {
        m_accessToken = other.m_accessToken;
        m_refreshToken = other.m_refreshToken;
        m_openIdToken = other.m_openIdToken;
        m_providerName = other.m_providerName;
        m_tokensExpireTimeSeconds = other.m_tokensExpireTimeSeconds;
        m_tokensExpireTimeStamp = other.m_tokensExpireTimeStamp;
    }

    AuthenticationTokens::AuthenticationTokens(
        const AZStd::string& accessToken, const AZStd::string& refreshToken, const AZStd::string& openidToken, const ProviderNameEnum& providerName, int tokensExpireTimeSeconds)
    {
        m_accessToken = accessToken;
        m_refreshToken = refreshToken;
        m_openIdToken = openidToken;
        m_providerName = providerName;
        m_tokensExpireTimeSeconds = tokensExpireTimeSeconds;
        m_tokensExpireTimeStamp =  AZStd::chrono::system_clock::now() + AZStd::chrono::seconds(tokensExpireTimeSeconds);
    }

    //! Compares current time stamp to expired time stamp.
    //! @return True if current TS less than expiry TS.
    bool AuthenticationTokens::AreTokensValid() const
    {
        return AZStd::chrono::system_clock::now() < m_tokensExpireTimeStamp;
    }

    //! @return Open id token from authentication.
    AZStd::string AuthenticationTokens::GetOpenIdToken() const
    {
        return m_openIdToken;
    }

    //! @return Access token from authentication.
    AZStd::string AuthenticationTokens::GetAccessToken() const
    {
        return m_accessToken;
    }

    //! @return Refresh token from authentication.
    AZStd::string AuthenticationTokens::GetRefreshToken() const
    {
        return m_refreshToken;
    }

    //! @return Provide name for the tokens.
    ProviderNameEnum AuthenticationTokens::GetProviderName() const
    {
        return m_providerName;
    }

    //! @return Expiration time in seconds.
    int AuthenticationTokens::GetTokensExpireTimeSeconds() const
    {
        return m_tokensExpireTimeSeconds;
    }
} // namespace AWSClientAuth
