/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Preprocessor/Enum.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/chrono/clocks.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>

namespace AWSClientAuth
{
    AZ_ENUM_CLASS(ProviderNameEnum, None, AWSCognitoIDP, LoginWithAmazon, Twitch, Google, Apple, Facebook);

    //! Used to share authentication tokens to caller and to AWSCognitoAuthorizationController.
    class AuthenticationTokens
    {
    public:
        AZ_TYPE_INFO(AuthenticationTokens, "{F965D1B2-9DE3-4900-B44B-E58D9F083ACB}");
        AuthenticationTokens();
        AuthenticationTokens(const AuthenticationTokens& other);
        AuthenticationTokens(const AZStd::string& accessToken, const AZStd::string& refreshToken, const AZStd::string& openidToken
            , const ProviderNameEnum& providerName, int tokensExpireTimeSeconds);

        //! Compares current time stamp to expired time stamp.
        //! @return True if current TS less than expiry TS.
        bool AreTokensValid() const;

        //! @return Open id token from authentication.
        AZStd::string GetOpenIdToken() const;

        //! @return Access token from authentication.
        AZStd::string GetAccessToken() const;

        //! @return Refresh token from authentication.
        AZStd::string GetRefreshToken() const;

        //! @return Provide name for the tokens.
        ProviderNameEnum GetProviderName() const;

        //! @return Expiration time in seconds.
        int GetTokensExpireTimeSeconds() const;

        static void Reflect(AZ::ReflectContext* context);

    private:
        int m_tokensExpireTimeSeconds = 0;
        AZStd::string m_accessToken;
        AZStd::string m_refreshToken;
        AZStd::string m_openIdToken;
        ProviderNameEnum m_providerName;
        AZStd::chrono::system_clock::time_point m_tokensExpireTimeStamp;
    };
} // namespace AWSClientAuth
