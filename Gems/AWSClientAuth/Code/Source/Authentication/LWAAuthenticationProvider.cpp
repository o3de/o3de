/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
    constexpr char LwaSettingsPath[] = "/AWS/LoginWithAmazon";    
    constexpr char LwaVerificationUrlResponseKey[] = "verification_uri";

    LWAAuthenticationProvider::LWAAuthenticationProvider()
    {
        m_settings = AZStd::make_unique<LWAProviderSetting>();
    }

    LWAAuthenticationProvider::~LWAAuthenticationProvider()
    {
        m_settings.reset();
    }

    bool LWAAuthenticationProvider::Initialize()
    {
        AZ::SettingsRegistryInterface* settingsRegistry = AZ::SettingsRegistry::Get();
        if (!settingsRegistry)
        {
            AZ_Warning("AWSCognitoAuthenticationProvider", false, "Failed to load the setting registry");
            return false;
        }

        if (!settingsRegistry->GetObject(m_settings.get(), azrtti_typeid(m_settings.get()), LwaSettingsPath))
        {
            AZ_Warning("AWSCognitoAuthenticationProvider", false, "Failed to get login with Amazon settings object for path %s", LwaSettingsPath);
            return false;
        }
        return true;
    }

    void LWAAuthenticationProvider::PasswordGrantSingleFactorSignInAsync(const AZStd::string& username, const AZStd::string& password)
    {
        AZ_UNUSED(username);
        AZ_UNUSED(password);
        AZ_Assert(false, "Not supported");
    }

    void LWAAuthenticationProvider::PasswordGrantMultiFactorSignInAsync(const AZStd::string& username, const AZStd::string& password)
    {
        AZ_UNUSED(username);
        AZ_UNUSED(password);
        AZ_Assert(false, "Not supported");
    }

    void LWAAuthenticationProvider::PasswordGrantMultiFactorConfirmSignInAsync(const AZStd::string& username, const AZStd::string& confirmationCode)
    {
        AZ_UNUSED(username);
        AZ_UNUSED(confirmationCode);
        AZ_Assert(false, "Not supported");
    }

    // Call LWA authentication provider device code end point.
    // Refer https://developer.amazon.com/docs/login-with-amazon/retrieve-code-other-platforms-cbl-docs.html.
    void LWAAuthenticationProvider::DeviceCodeGrantSignInAsync()
    {
        // Set headers and body for device sign in http requests.
        AZStd::string body = AZStd::string::format("%s=%s&%s=%s&%s=%s", OAuthResponseTypeBodyKey, m_settings->m_responseType.c_str()
            , OAuthClientIdBodyKey, m_settings->m_appClientId.c_str(), OAuthScopeBodyKey, OAuthScopeBodyValue);

        AZStd::map<AZStd::string, AZStd::string> headers;
        headers[OAuthContentTypeHeaderKey] = OAuthContentTypeHeaderValue;
        headers[OAuthContentLengthHeaderKey] = AZStd::to_string(body.length());
        
        HttpRequestor::HttpRequestorRequestBus::Broadcast(&HttpRequestor::HttpRequestorRequests::AddRequestWithHeadersAndBody, m_settings->m_oAuthCodeURL
            , Aws::Http::HttpMethod::HTTP_POST, headers, body
            , [this](const Aws::Utils::Json::JsonView& jsonView, Aws::Http::HttpResponseCode responseCode)
            {
                if (responseCode == Aws::Http::HttpResponseCode::OK)
                {
                    m_cachedUserCode = jsonView.GetString(OAuthUserCodeResponseKey).c_str();
                    m_cachedDeviceCode = jsonView.GetString(OAuthDeviceCodeBodyKey).c_str();
                    AuthenticationProviderNotificationBus::Broadcast(&AuthenticationProviderNotifications::OnDeviceCodeGrantSignInSuccess
                        , jsonView.GetString(OAuthUserCodeResponseKey).c_str()
                        , jsonView.GetString(LwaVerificationUrlResponseKey).c_str()
                        , jsonView.GetInteger(OAuthExpiresInResponseKey));
                }
                else
                {
                    AuthenticationProviderNotificationBus::Broadcast(&AuthenticationProviderNotifications::OnDeviceCodeGrantSignInFail
                        , jsonView.GetString(OAuthErrorResponseKey).c_str());
                }
            }
        );
    }

    // Call LWA authentication provider OAuth tokens endpoint
    // Refer https://developer.amazon.com/docs/login-with-amazon/retrieve-token-other-platforms-cbl-docs.html
    void LWAAuthenticationProvider::DeviceCodeGrantConfirmSignInAsync()
    {
        // Set headers and body for device confirm sign in http requests.
        AZStd::string body = AZStd::string::format("%s=%s&%s=%s&%s=%s", OAuthUserCodeResponseKey, m_cachedUserCode.c_str()
            , OAuthGrantTypeBodyKey, m_settings->m_grantType.c_str(), OAuthDeviceCodeBodyKey, m_cachedDeviceCode.c_str());

        AZStd::map<AZStd::string, AZStd::string> headers;
        headers[OAuthContentTypeHeaderKey] = OAuthContentTypeHeaderValue;
        headers[OAuthContentLengthHeaderKey] = AZStd::to_string(body.length());

        HttpRequestor::HttpRequestorRequestBus::Broadcast(&HttpRequestor::HttpRequestorRequests::AddRequestWithHeadersAndBody, m_settings->m_oAuthTokensURL
            , Aws::Http::HttpMethod::HTTP_POST, headers, body
            , [this](const Aws::Utils::Json::JsonView& jsonView, Aws::Http::HttpResponseCode responseCode)
            {
                if (responseCode == Aws::Http::HttpResponseCode::OK)
                {
                    // Id and access token are the same.
                    UpdateTokens(jsonView);

                    AuthenticationProviderNotificationBus::Broadcast(&AuthenticationProviderNotifications::OnDeviceCodeGrantConfirmSignInSuccess,
                        AuthenticationTokens(jsonView.GetString(OAuthAccessTokenResponseKey).c_str(),
                            jsonView.GetString(OAuthRefreshTokenResponseKey).c_str(), jsonView.GetString(OAuthAccessTokenResponseKey).c_str(),
                            ProviderNameEnum::LoginWithAmazon, jsonView.GetInteger(OAuthExpiresInResponseKey)));
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
        AZStd::string body = AZStd::string::format("%s=%s&%s=%s&%s=%s", OAuthClientIdBodyKey, m_settings->m_appClientId.c_str(), OAuthGrantTypeBodyKey,
            OAuthRefreshTokenBodyValue, OAuthRefreshTokenBodyKey, m_authenticationTokens.GetRefreshToken().c_str());

        AZStd::map<AZStd::string, AZStd::string> headers;
        headers[OAuthContentTypeHeaderKey] = OAuthContentTypeHeaderValue;
        headers[OAuthContentLengthHeaderKey] = AZStd::to_string(body.length());

        HttpRequestor::HttpRequestorRequestBus::Broadcast(&HttpRequestor::HttpRequestorRequests::AddRequestWithHeadersAndBody, m_settings->m_oAuthTokensURL
            , Aws::Http::HttpMethod::HTTP_POST, headers, body
            , [this](const Aws::Utils::Json::JsonView& jsonView, Aws::Http::HttpResponseCode responseCode)
        {
            if (responseCode == Aws::Http::HttpResponseCode::OK)
            {
                // Id and access token are the same.
                UpdateTokens(jsonView);

                AuthenticationProviderNotificationBus::Broadcast(&AuthenticationProviderNotifications::OnRefreshTokensSuccess,
                    AuthenticationTokens(jsonView.GetString(OAuthAccessTokenResponseKey).c_str(),
                        jsonView.GetString(OAuthRefreshTokenResponseKey).c_str(), jsonView.GetString(OAuthAccessTokenResponseKey).c_str(),
                        ProviderNameEnum::LoginWithAmazon, jsonView.GetInteger(OAuthExpiresInResponseKey)));
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
        // Storing authentication tokens in memory can be a security concern. The access token and id token are not actually in use by
        // the authentication provider and shouldn't be stored in the member variable.
        m_authenticationTokens = AuthenticationTokens("", jsonView.GetString(OAuthRefreshTokenResponseKey).c_str(),
            "", ProviderNameEnum::LoginWithAmazon
            , jsonView.GetInteger(OAuthExpiresInResponseKey));
    }

} // namespace AWSClientAuth
