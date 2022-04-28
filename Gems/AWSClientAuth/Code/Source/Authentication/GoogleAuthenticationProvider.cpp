/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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

    constexpr char GoogleSettingsPath[] = "/AWS/Google";
    constexpr char GoogleVerificationUrlResponseKey[] = "verification_url";

    GoogleAuthenticationProvider::GoogleAuthenticationProvider()
    {
        m_settings = AZStd::make_unique<GoogleProviderSetting>();
    }

    GoogleAuthenticationProvider::~GoogleAuthenticationProvider()
    {
        m_settings.reset();
    }

    bool GoogleAuthenticationProvider::Initialize()
    {
        AZ::SettingsRegistryInterface* settingsRegistry = AZ::SettingsRegistry::Get();
        if (!settingsRegistry)
        {
            AZ_Warning("AWSCognitoAuthenticationProvider", false, "Failed to load the setting registry");
            return false;
        }

        if (!settingsRegistry->GetObject(m_settings.get(), azrtti_typeid(m_settings.get()), GoogleSettingsPath))
        {
            AZ_Warning("AWSCognitoAuthenticationProvider", false, "Failed to get Google settings object for path %s", GoogleSettingsPath);
            return false;
        }
        return true;
    }

    void GoogleAuthenticationProvider::PasswordGrantSingleFactorSignInAsync(const AZStd::string& username, const AZStd::string& password)
    {
        AZ_UNUSED(username);
        AZ_UNUSED(password);
        AZ_Assert(false, "Not supported");
    }

    void GoogleAuthenticationProvider::PasswordGrantMultiFactorSignInAsync(const AZStd::string& username, const AZStd::string& password)
    {
        AZ_UNUSED(username);
        AZ_UNUSED(password);
        AZ_Assert(false, "Not supported");
    }

    void GoogleAuthenticationProvider::PasswordGrantMultiFactorConfirmSignInAsync(const AZStd::string& username, const AZStd::string& confirmationCode)
    {
        AZ_UNUSED(username);
        AZ_UNUSED(confirmationCode);
        AZ_Assert(false, "Not supported");
    }

    // Call Google authentication provider device code end point.
    // Refer https://developers.google.com/identity/protocols/oauth2/limited-input-device#step-1:-request-device-and-user-codes.
    void GoogleAuthenticationProvider::DeviceCodeGrantSignInAsync()
    {
        AZStd::string body = AZStd::string::format("%s=%s&%s=%s", OAuthClientIdBodyKey, m_settings->m_appClientId.c_str()
            , OAuthScopeBodyKey, OAuthScopeBodyValue);

        // Set headers and body for device sign in http requests.
        AZStd::map<AZStd::string, AZStd::string> headers;
        headers[OAuthContentTypeHeaderKey] = OAuthContentTypeHeaderValue;
        headers[OAuthContentLengthHeaderKey] = AZStd::to_string(body.length());

        HttpRequestor::HttpRequestorRequestBus::Broadcast(&HttpRequestor::HttpRequestorRequests::AddRequestWithHeadersAndBody, m_settings->m_oAuthCodeURL
            , Aws::Http::HttpMethod::HTTP_POST, headers, body
            , [this](const Aws::Utils::Json::JsonView& jsonView, Aws::Http::HttpResponseCode responseCode)
            {
                if (responseCode == Aws::Http::HttpResponseCode::OK)
                {
                    m_cachedDeviceCode = jsonView.GetString(OAuthDeviceCodeBodyKey).c_str();
                    AuthenticationProviderNotificationBus::Broadcast(&AuthenticationProviderNotifications::OnDeviceCodeGrantSignInSuccess
                        , jsonView.GetString(OAuthUserCodeResponseKey).c_str(), jsonView.GetString(GoogleVerificationUrlResponseKey).c_str()
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


    // Call Google authentication provider OAuth tokens endpoint
    // Refer https://developers.google.com/identity/protocols/oauth2/limited-input-device#step-4:-poll-googles-authorization-server.
    void GoogleAuthenticationProvider::DeviceCodeGrantConfirmSignInAsync()
    {
        // Set headers and body for device confirm sign in http requests.
        AZStd::map<AZStd::string, AZStd::string> headers;
        AZStd::string body = AZStd::string::format("%s=%s&%s=%s&%s=%s&%s=%s", OAuthClientIdBodyKey, m_settings->m_appClientId.c_str()
            , OAuthClientSecretBodyKey, m_settings->m_clientSecret.c_str(), OAuthDeviceCodeBodyKey, m_cachedDeviceCode.c_str()
            , OAuthGrantTypeBodyKey, m_settings->m_grantType.c_str());
 
        headers[OAuthContentTypeHeaderKey] = OAuthContentTypeHeaderValue;
        headers[OAuthContentLengthHeaderKey] = AZStd::to_string(body.length());

        HttpRequestor::HttpRequestorRequestBus::Broadcast(&HttpRequestor::HttpRequestorRequests::AddRequestWithHeadersAndBody, m_settings->m_oAuthTokensURL
            , Aws::Http::HttpMethod::HTTP_POST, headers, body
            , [this](const Aws::Utils::Json::JsonView& jsonView, Aws::Http::HttpResponseCode responseCode)
            {
                if (responseCode == Aws::Http::HttpResponseCode::OK)
                {
                    UpdateTokens(jsonView);

                    AuthenticationProviderNotificationBus::Broadcast(
                        &AuthenticationProviderNotifications::OnDeviceCodeGrantConfirmSignInSuccess,
                        AuthenticationTokens(jsonView.GetString(OAuthAccessTokenResponseKey).c_str(),
                            jsonView.GetString(OAuthRefreshTokenResponseKey).c_str(), jsonView.GetString(OAuthIdTokenResponseKey).c_str(),
                            ProviderNameEnum::Google, jsonView.GetInteger(OAuthExpiresInResponseKey)));
                }
                else
                {
                    AuthenticationProviderNotificationBus::Broadcast(&AuthenticationProviderNotifications::OnDeviceCodeGrantConfirmSignInFail
                        , jsonView.GetString(OAuthErrorResponseKey).c_str());
                }
            }
        );
    }

    // Call Google authentication provider OAuth tokens endpoint
    // Refer https://developers.google.com/identity/protocols/oauth2/limited-input-device#offline.
    void GoogleAuthenticationProvider::RefreshTokensAsync()
    {
        AZStd::map<AZStd::string, AZStd::string> headers;
        AZStd::string body = AZStd::string::format("%s=%s&%s=%s&%s=%s&%s=%s", OAuthClientIdBodyKey, m_settings->m_appClientId.c_str()
            , OAuthClientSecretBodyKey, m_settings->m_clientSecret.c_str()
            , OAuthGrantTypeBodyKey, OAuthRefreshTokenBodyValue, OAuthRefreshTokenBodyKey, m_authenticationTokens.GetRefreshToken().c_str());

        headers[OAuthContentTypeHeaderKey] = OAuthContentTypeHeaderValue;
        headers[OAuthContentLengthHeaderKey] = AZStd::to_string(body.length());

        HttpRequestor::HttpRequestorRequestBus::Broadcast(&HttpRequestor::HttpRequestorRequests::AddRequestWithHeadersAndBody, m_settings->m_oAuthTokensURL
            , Aws::Http::HttpMethod::HTTP_POST, headers, body
            , [this](const Aws::Utils::Json::JsonView& jsonView, Aws::Http::HttpResponseCode responseCode)
        {
            if (responseCode == Aws::Http::HttpResponseCode::OK)
            {
                UpdateTokens(jsonView);

                AuthenticationProviderNotificationBus::Broadcast(&AuthenticationProviderNotifications::OnRefreshTokensSuccess,
                    AuthenticationTokens(jsonView.GetString(OAuthAccessTokenResponseKey).c_str(),
                        jsonView.GetString(OAuthRefreshTokenResponseKey).c_str() ,jsonView.GetString(OAuthIdTokenResponseKey).c_str(),
                        ProviderNameEnum::Google, jsonView.GetInteger(OAuthExpiresInResponseKey)));
            }
            else
            {
                AuthenticationProviderNotificationBus::Broadcast(&AuthenticationProviderNotifications::OnRefreshTokensFail
                    , jsonView.GetString(OAuthErrorResponseKey).c_str());
            }
        }
        );
    }

    void GoogleAuthenticationProvider::UpdateTokens(const Aws::Utils::Json::JsonView& jsonView)
    {
        // Storing authentication tokens in memory can be a security concern. The access token and id token are not actually in use by
        // the authentication provider and shouldn't be stored in the member variable.
        m_authenticationTokens = AuthenticationTokens("",
            jsonView.GetString(OAuthRefreshTokenResponseKey).c_str() ,"", ProviderNameEnum::Google
            , jsonView.GetInteger(OAuthExpiresInResponseKey));
    }

} // namespace AWSClientAuth
