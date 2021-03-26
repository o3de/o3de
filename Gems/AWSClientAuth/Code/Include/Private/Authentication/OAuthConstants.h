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


namespace AWSClientAuth
{
    constexpr char OAuthClientIdBodyKey[] = "client_id";
    constexpr char OAuthClientSecretBodyKey[] = "client_secret";
    constexpr char OAuthDeviceCodeBodyKey[] = "device_code";
    constexpr char OAuthScopeBodyKey[] = "scope";
    constexpr char OAuthScopeBodyValue[] = "profile";
    constexpr char OAuthGrantTypeBodyKey[] = "grant_type";
    constexpr char OAuthRefreshTokenBodyKey[] = "refresh_token";
    constexpr char OAuthRefreshTokenBodyValue[] = "refresh_token";
    constexpr char OAuthResponseTypeBodyKey[] = "response_type";

    constexpr char OAuthContentTypeHeaderKey[] = "Content-Type";
    constexpr char OAuthContentTypeHeaderValue[] = "application/x-www-form-urlencoded";
    constexpr char OAuthContentLengthHeaderKey[] = "Content-Length";

    constexpr char OAuthUserCodeResponseKey[] = "user_code";
    constexpr char OAuthIdTokenResponseKey[] = "id_token";
    constexpr char OAuthAccessTokenResponseKey[] = "access_token";
    constexpr char OAuthRefreshTokenResponseKey[] = "refresh_token";
    constexpr char OAuthExpiresInResponseKey[] = "expires_in";
    constexpr char OAuthErrorResponseKey[] = "error";

} // namespace AWSClientAuth
