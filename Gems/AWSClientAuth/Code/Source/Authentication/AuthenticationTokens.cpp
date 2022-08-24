/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
        : m_accessToken(other.m_accessToken)
        , m_refreshToken(other.m_refreshToken)
        , m_openIdToken(other.m_openIdToken)
        , m_providerName(other.m_providerName)
        , m_tokensExpireTimeSeconds(other.m_tokensExpireTimeSeconds)
        , m_tokensExpireTimeStamp(other.m_tokensExpireTimeStamp)
    {
    }

    AuthenticationTokens::AuthenticationTokens(
        const AZStd::string& accessToken, const AZStd::string& refreshToken, const AZStd::string& openIdToken, const ProviderNameEnum& providerName, int tokensExpireTimeSeconds)
        : m_accessToken(accessToken)
        , m_refreshToken(refreshToken)
        , m_openIdToken(openIdToken)
        , m_providerName(providerName)
        , m_tokensExpireTimeSeconds(tokensExpireTimeSeconds)
        , m_tokensExpireTimeStamp(AZStd::chrono::system_clock::now() + AZStd::chrono::seconds(tokensExpireTimeSeconds))
    {
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

    void AuthenticationTokens::Reflect(AZ::ReflectContext* context)
    {
        auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<AuthenticationTokens>()
                ->Field("AccessToken", &AuthenticationTokens::m_accessToken)
                ->Field("OpenIdToken", &AuthenticationTokens::m_openIdToken)
                ->Field("RefreshToken", &AuthenticationTokens::m_refreshToken);
        }

        AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
        if (behaviorContext)
        {
            behaviorContext->Class<AuthenticationTokens>()
                ->Attribute(AZ::Script::Attributes::Category, "AWSClientAuth")
                ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::Value)
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Constructor()
                ->Constructor<const AuthenticationTokens&>()
                ->Property("AccessToken", BehaviorValueGetter(&AuthenticationTokens::m_accessToken), BehaviorValueSetter(&AuthenticationTokens::m_accessToken))
                ->Property("OpenIdToken", BehaviorValueGetter(&AuthenticationTokens::m_openIdToken), BehaviorValueSetter(&AuthenticationTokens::m_accessToken))
                ->Property("RefreshToken", BehaviorValueGetter(&AuthenticationTokens::m_refreshToken), BehaviorValueSetter(&AuthenticationTokens::m_accessToken));
        }
    }
} // namespace AWSClientAuth
