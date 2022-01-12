/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
