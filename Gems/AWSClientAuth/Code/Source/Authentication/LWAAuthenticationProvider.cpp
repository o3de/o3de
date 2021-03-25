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
#include <Authentication/LWAAuthenticationProvider.h>
#include <Authentication/AuthenticationProviderBus.h>
#include <Authentication/OAuthConstants.h>
#include <HttpRequestor/HttpRequestorBus.h>
#include <HttpRequestor/HttpTypes.h>

#include <aws/core/http/HttpResponse.h>

namespace AWSClientAuth
{
    constexpr char LWA_SETTINGS_PATH[] = "/AWS/LoginWithAmazon";    
    constexpr char LWA_VERIFICATION_URL_RESPONSE_KEY[] = "verification_uri";

    LWAAuthenticationProvider::LWAAuthenticationProvider()
    {
        m_settings = AZStd::make_unique<LWAProviderSetting>();
    }

    LWAAuthenticationProvider::~LWAAuthenticationProvider()
    {
        m_settings.reset();
    }

    bool LWAAuthenticationProvider::Initialize(AZStd::weak_ptr<AZ::SettingsRegistryInterface> settingsRegistry)
    {
        if (!settingsRegistry.lock()->GetObject(m_settings.get(), azrtti_typeid(m_settings.get()), LWA_SETTINGS_PATH))
        {
            AZ_Warning("AWSCognitoAuthenticationProvider", true, "Failed to get login with Amazon settings object for path %s", LWA_SETTINGS_PATH);
            return false;
        }
        return true;
    }

    void LWAAuthenticationProvider::PasswordGrantSingleFactorSignInAsync(const AZStd::string& username, const AZStd::string& password)
    {
        AZ_UNUSED(username);
        AZ_UNUSED(password);
        AZ_Assert(true, "Not supported");
    }

    void LWAAuthenticationProvider::PasswordGrantMultiFactorSignInAsync(const AZStd::string& username, const AZStd::string& password)
    {
        AZ_UNUSED(username);
        AZ_UNUSED(password);
        AZ_Assert(true, "Not supported");
    }

    void LWAAuthenticationProvider::PasswordGrantMultiFactorConfirmSignInAsync(const AZStd::string& username, const AZStd::string& confirmationCode)
    {
        AZ_UNUSED(username);
        AZ_UNUSED(confirmationCode);
        AZ_Assert(true, "Not supported");
    }

    // Call LWA authentication provider device code end point.
    // Refer https://developer.amazon.com/docs/login-with-amazon/retrieve-code-other-platforms-cbl-docs.html.
    void LWAAuthenticationProvider::DeviceCodeGrantSignInAsync()
    {
        // Set headers and body for device sign in http requests.
        AZStd::string body = AZStd::string::format("%s=%s&%s=%s&%s=%s", OAUTH_RESPONSE_TYPE_BODY_KEY, m_settings->m_responseType.c_str()
            , OAUTH_CLIENT_ID_BODY_KEY, m_settings->m_appClientId.c_str(), OAUTH_SCOPE_BODY_KEY, OAUTH_SCOPE_BODY_VALUE);

        AZStd::map<AZStd::string, AZStd::string> headers;
        headers[OAUTH_CONTENT_TYPE_HEADER_KEY] = OAUTH_CONTENT_TYPE_HEADER_VALUE;
        headers[OAUTH_CONTENT_LENGTH_HEADER_KEY] = AZStd::to_string(body.length());
        
        HttpRequestor::HttpRequestorRequestBus::Broadcast(&HttpRequestor::HttpRequestorRequests::AddRequestWithHeadersAndBody, m_settings->m_oAuthCodeURL
            , Aws::Http::HttpMethod::HTTP_POST, headers, body
            , [this](const Aws::Utils::Json::JsonView& jsonView, Aws::Http::HttpResponseCode responseCode)
            {
                if (responseCode == Aws::Http::HttpResponseCode::OK)
                {
                    m_cachedUserCode = jsonView.GetString(OAUTH_USER_CODE_RESPONSE_KEY).c_str();
                    m_cachedDeviceCode = jsonView.GetString(OAUTH_DEVICE_CODE_BODY_KEY).c_str();
                    AuthenticationProviderNotificationBus::Broadcast(&AuthenticationProviderNotifications::OnDeviceCodeGrantSignInSuccess
                        , jsonView.GetString(OAUTH_USER_CODE_RESPONSE_KEY).c_str()
                        , jsonView.GetString(LWA_VERIFICATION_URL_RESPONSE_KEY).c_str()
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

    // Call LWA authentication provider OAuth tokens endpoint
    // Refer https://developer.amazon.com/docs/login-with-amazon/retrieve-token-other-platforms-cbl-docs.html
    void LWAAuthenticationProvider::DeviceCodeGrantConfirmSignInAsync()
    {
        // Set headers and body for device confirm sign in http requests.
        AZStd::string body = AZStd::string::format("%s=%s&%s=%s&%s=%s", OAUTH_USER_CODE_RESPONSE_KEY, m_cachedUserCode.c_str()
            , OAUTH_GRANT_TYPE_BODY_KEY, m_settings->m_grantType.c_str(), OAUTH_DEVICE_CODE_BODY_KEY, m_cachedDeviceCode.c_str());

        AZStd::map<AZStd::string, AZStd::string> headers;
        headers[OAUTH_CONTENT_TYPE_HEADER_KEY] = OAUTH_CONTENT_TYPE_HEADER_VALUE;
        headers[OAUTH_CONTENT_LENGTH_HEADER_KEY] = AZStd::to_string(body.length());

        HttpRequestor::HttpRequestorRequestBus::Broadcast(&HttpRequestor::HttpRequestorRequests::AddRequestWithHeadersAndBody, m_settings->m_oAuthTokensURL
            , Aws::Http::HttpMethod::HTTP_POST, headers, body
            , [this](const Aws::Utils::Json::JsonView& jsonView, Aws::Http::HttpResponseCode responseCode)
            {
                if (responseCode == Aws::Http::HttpResponseCode::OK)
                {
                    // Id and access token are the same.
                    UpdateTokens(jsonView);
                    AuthenticationProviderNotificationBus::Broadcast(
                        &AuthenticationProviderNotifications::OnDeviceCodeGrantConfirmSignInSuccess, m_authenticationTokens);
                    m_cachedUserCode = "";
                    m_cachedDeviceCode = "";
                }
                else
                {
                    AuthenticationProviderNotificationBus::Broadcast(&AuthenticationProviderNotifications::OnDeviceCodeGrantConfirmSignInFail
                        , jsonView.GetString("error").c_str());
                }
            }
        );
    }

    void LWAAuthenticationProvider::RefreshTokensAsync()
    {
        // Set headers and body for device confirm sign in http requests.
        AZStd::string body = AZStd::string::format("%s=%s&%s=%s&%s=%s", OAUTH_CLIENT_ID_BODY_KEY, m_settings->m_appClientId.c_str(), OAUTH_GRANT_TYPE_BODY_KEY,
            OAUTH_REFRESH_TOKEN_BODY_VALUE, OAUTH_REFRESH_TOKEN_BODY_KEY, m_authenticationTokens.GetRefreshToken().c_str());

        AZStd::map<AZStd::string, AZStd::string> headers;
        headers[OAUTH_CONTENT_TYPE_HEADER_KEY] = OAUTH_CONTENT_TYPE_HEADER_VALUE;
        headers[OAUTH_CONTENT_LENGTH_HEADER_KEY] = AZStd::to_string(body.length());

        HttpRequestor::HttpRequestorRequestBus::Broadcast(&HttpRequestor::HttpRequestorRequests::AddRequestWithHeadersAndBody, m_settings->m_oAuthTokensURL
            , Aws::Http::HttpMethod::HTTP_POST, headers, body
            , [this](const Aws::Utils::Json::JsonView& jsonView, Aws::Http::HttpResponseCode responseCode)
        {
            if (responseCode == Aws::Http::HttpResponseCode::OK)
            {
                // Id and access token are the same.
                UpdateTokens(jsonView);
                AuthenticationProviderNotificationBus::Broadcast(&AuthenticationProviderNotifications::OnRefreshTokensSuccess, m_authenticationTokens);
            }
            else
            {
                AuthenticationProviderNotificationBus::Broadcast(&AuthenticationProviderNotifications::OnRefreshTokensFail
                    , jsonView.GetString("error").c_str());
            }
        }
        );
    }

    void LWAAuthenticationProvider::UpdateTokens(const Aws::Utils::Json::JsonView& jsonView)
    {
        // For Login with Amazon openId and access tokens are the same.
        m_authenticationTokens = AuthenticationTokens(jsonView.GetString(OAUTH_ACCESS_TOKEN_RESPONSE_KEY).c_str(), jsonView.GetString(OAUTH_REFRESH_TOKEN_RESPONSE_KEY).c_str(),
            jsonView.GetString(OAUTH_ACCESS_TOKEN_RESPONSE_KEY).c_str(), ProviderNameEnum::LoginWithAmazon
            , jsonView.GetInteger(OAUTH_EXPIRES_IN_RESPONSE_KEY));
    }

} // namespace AWSClientAuth
