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

#include <AzCore/std/smart_ptr/make_shared.h>
#include <Authentication/GoogleAuthenticationProvider.h>
#include <Authentication/AuthenticationProviderBus.h>
#include <Authentication/OAuthConstants.h>
#include <HttpRequestor/HttpRequestorBus.h>
#include <HttpRequestor/HttpTypes.h>

#include <aws/core/http/HttpResponse.h>

namespace AWSClientAuth
{

    constexpr char GOOGLE_SETTINGS_PATH[] = "/AWS/Google";
    constexpr char GOOGLE_VERIFICATION_URL_RESPONSE_KEY[] = "verification_url";

    GoogleAuthenticationProvider::GoogleAuthenticationProvider()
    {
        m_settings = AZStd::make_unique<GoogleProviderSetting>();
    }

    GoogleAuthenticationProvider::~GoogleAuthenticationProvider()
    {
        m_settings.reset();
    }

    bool GoogleAuthenticationProvider::Initialize(AZStd::weak_ptr<AZ::SettingsRegistryInterface> settingsRegistry)
    {
        if (!settingsRegistry.lock()->GetObject(m_settings.get(), azrtti_typeid(m_settings.get()), GOOGLE_SETTINGS_PATH))
        {
            AZ_Warning("AWSCognitoAuthenticationProvider", true, "Failed to get Google settings object for path %s", GOOGLE_SETTINGS_PATH);
            return false;
        }
        return true;
    }

    void GoogleAuthenticationProvider::PasswordGrantSingleFactorSignInAsync(const AZStd::string& username, const AZStd::string& password)
    {
        AZ_UNUSED(username);
        AZ_UNUSED(password);
        AZ_Assert(true, "Not supported");
    }

    void GoogleAuthenticationProvider::PasswordGrantMultiFactorSignInAsync(const AZStd::string& username, const AZStd::string& password)
    {
        AZ_UNUSED(username);
        AZ_UNUSED(password);
        AZ_Assert(true, "Not supported");
    }

    void GoogleAuthenticationProvider::PasswordGrantMultiFactorConfirmSignInAsync(const AZStd::string& username, const AZStd::string& confirmationCode)
    {
        AZ_UNUSED(username);
        AZ_UNUSED(confirmationCode);
        AZ_Assert(true, "Not supported");
    }

    // Call Google authentication provider device code end point.
    // Refer https://developers.google.com/identity/protocols/oauth2/limited-input-device#step-1:-request-device-and-user-codes.
    void GoogleAuthenticationProvider::DeviceCodeGrantSignInAsync()
    {
        AZStd::string body = AZStd::string::format("%s=%s&%s=%s", OAUTH_CLIENT_ID_BODY_KEY, m_settings->m_appClientId.c_str()
            , OAUTH_SCOPE_BODY_KEY, OAUTH_SCOPE_BODY_VALUE);

        // Set headers and body for device sign in http requests.
        AZStd::map<AZStd::string, AZStd::string> headers;
        headers[OAUTH_CONTENT_TYPE_HEADER_KEY] = OAUTH_CONTENT_TYPE_HEADER_VALUE;
        headers[OAUTH_CONTENT_LENGTH_HEADER_KEY] = AZStd::to_string(body.length());

        HttpRequestor::HttpRequestorRequestBus::Broadcast(&HttpRequestor::HttpRequestorRequests::AddRequestWithHeadersAndBody, m_settings->m_oAuthCodeURL
            , Aws::Http::HttpMethod::HTTP_POST, headers, body
            , [this](const Aws::Utils::Json::JsonView& jsonView, Aws::Http::HttpResponseCode responseCode)
            {
                if (responseCode == Aws::Http::HttpResponseCode::OK)
                {
                    m_cachedDeviceCode = jsonView.GetString(OAUTH_DEVICE_CODE_BODY_KEY).c_str();
                    AuthenticationProviderNotificationBus::Broadcast(&AuthenticationProviderNotifications::OnDeviceCodeGrantSignInSuccess
                        , jsonView.GetString(OAUTH_USER_CODE_RESPONSE_KEY).c_str(), jsonView.GetString(GOOGLE_VERIFICATION_URL_RESPONSE_KEY).c_str()
                        , jsonView.GetInteger(OAUTH_EXPIRES_IN_RESPONSE_KEY));
                }
                else
                {
                    AuthenticationProviderNotificationBus::Broadcast(&AuthenticationProviderNotifications::OnDeviceCodeGrantSignInFail
                        , jsonView.GetString(OAUTH_ERROR_RESPONSE_KEY).c_str());
                }
            }
        );
    }


    // Call Google authentication provider OAuth tokens endpoint
    // Refer https://developers.google.com/identity/protocols/oauth2/limited-input-device#step-4:-poll-googles-authorization-server.
    void GoogleAuthenticationProvider::DeviceCodeGrantConfirmSignInAsync()
    {
        // Set headers and body for device confirm sign in http requests.
        AZStd::map<AZStd::string, AZStd::string> headers;
        AZStd::string body = AZStd::string::format("%s=%s&%s=%s&%s=%s&%s=%s", OAUTH_CLIENT_ID_BODY_KEY, m_settings->m_appClientId.c_str()
            , OAUTH_CLIENT_SECRET_BODY_KEY, m_settings->m_clientSecret.c_str(), OAUTH_DEVICE_CODE_BODY_KEY, m_cachedDeviceCode.c_str()
            , OAUTH_GRANT_TYPE_BODY_KEY, m_settings->m_grantType.c_str());
 
        headers[OAUTH_CONTENT_TYPE_HEADER_KEY] = OAUTH_CONTENT_TYPE_HEADER_VALUE;
        headers[OAUTH_CONTENT_LENGTH_HEADER_KEY] = AZStd::to_string(body.length());

        HttpRequestor::HttpRequestorRequestBus::Broadcast(&HttpRequestor::HttpRequestorRequests::AddRequestWithHeadersAndBody, m_settings->m_oAuthTokensURL
            , Aws::Http::HttpMethod::HTTP_POST, headers, body
            , [this](const Aws::Utils::Json::JsonView& jsonView, Aws::Http::HttpResponseCode responseCode)
            {
                if (responseCode == Aws::Http::HttpResponseCode::OK)
                {
                    UpdateTokens(jsonView);
                    AuthenticationProviderNotificationBus::Broadcast(
                        &AuthenticationProviderNotifications::OnDeviceCodeGrantConfirmSignInSuccess, m_authenticationTokens);
                }
                else
                {
                    AuthenticationProviderNotificationBus::Broadcast(&AuthenticationProviderNotifications::OnDeviceCodeGrantConfirmSignInFail
                        , jsonView.GetString(OAUTH_ERROR_RESPONSE_KEY).c_str());
                }
            }
        );
    }

    // Call Google authentication provider OAuth tokens endpoint
    // Refer https://developers.google.com/identity/protocols/oauth2/limited-input-device#offline.
    void GoogleAuthenticationProvider::RefreshTokensAsync()
    {
        AZStd::map<AZStd::string, AZStd::string> headers;
        AZStd::string body = AZStd::string::format("%s=%s&%s=%s&%s=%s&%s=%s", OAUTH_CLIENT_ID_BODY_KEY, m_settings->m_appClientId.c_str()
            , OAUTH_CLIENT_SECRET_BODY_KEY, m_settings->m_clientSecret.c_str()
            , OAUTH_GRANT_TYPE_BODY_KEY, OAUTH_REFRESH_TOKEN_BODY_VALUE, OAUTH_REFRESH_TOKEN_BODY_KEY, m_authenticationTokens.GetRefreshToken().c_str());

        headers[OAUTH_CONTENT_TYPE_HEADER_KEY] = OAUTH_CONTENT_TYPE_HEADER_VALUE;
        headers[OAUTH_CONTENT_LENGTH_HEADER_KEY] = AZStd::to_string(body.length());

        HttpRequestor::HttpRequestorRequestBus::Broadcast(&HttpRequestor::HttpRequestorRequests::AddRequestWithHeadersAndBody, m_settings->m_oAuthTokensURL
            , Aws::Http::HttpMethod::HTTP_POST, headers, body
            , [this](const Aws::Utils::Json::JsonView& jsonView, Aws::Http::HttpResponseCode responseCode)
        {
            if (responseCode == Aws::Http::HttpResponseCode::OK)
            {
                UpdateTokens(jsonView);
                AuthenticationProviderNotificationBus::Broadcast(
                    &AuthenticationProviderNotifications::OnRefreshTokensSuccess, m_authenticationTokens);
            }
            else
            {
                AuthenticationProviderNotificationBus::Broadcast(&AuthenticationProviderNotifications::OnRefreshTokensFail
                    , jsonView.GetString(OAUTH_ERROR_RESPONSE_KEY).c_str());
            }
        }
        );
    }

    void GoogleAuthenticationProvider::UpdateTokens(const Aws::Utils::Json::JsonView& jsonView)
    {
        m_authenticationTokens = AuthenticationTokens(jsonView.GetString(OAUTH_ACCESS_TOKEN_RESPONSE_KEY).c_str(),
            jsonView.GetString(OAUTH_REFRESH_TOKEN_RESPONSE_KEY).c_str() ,jsonView.GetString(OAUTH_ID_TOKEN_RESPONSE_KEY).c_str(), ProviderNameEnum::Google
            , jsonView.GetInteger(OAUTH_EXPIRES_IN_RESPONSE_KEY));
    }

} // namespace AWSClientAuth
