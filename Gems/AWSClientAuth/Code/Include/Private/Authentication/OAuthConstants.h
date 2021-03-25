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
    constexpr char OAUTH_CLIENT_ID_BODY_KEY[] = "client_id";
    constexpr char OAUTH_CLIENT_SECRET_BODY_KEY[] = "client_secret";
    constexpr char OAUTH_DEVICE_CODE_BODY_KEY[] = "device_code";
    constexpr char OAUTH_SCOPE_BODY_KEY[] = "scope";
    constexpr char OAUTH_SCOPE_BODY_VALUE[] = "profile";
    constexpr char OAUTH_GRANT_TYPE_BODY_KEY[] = "grant_type";
    constexpr char OAUTH_REFRESH_TOKEN_BODY_KEY[] = "refresh_token";
    constexpr char OAUTH_REFRESH_TOKEN_BODY_VALUE[] = "refresh_token";
    constexpr char OAUTH_RESPONSE_TYPE_BODY_KEY[] = "response_type";

    constexpr char OAUTH_CONTENT_TYPE_HEADER_KEY[] = "Content-Type";
    constexpr char OAUTH_CONTENT_TYPE_HEADER_VALUE[] = "application/x-www-form-urlencoded";
    constexpr char OAUTH_CONTENT_LENGTH_HEADER_KEY[] = "Content-Length";

    constexpr char OAUTH_USER_CODE_RESPONSE_KEY[] = "user_code";
    constexpr char OAUTH_ID_TOKEN_RESPONSE_KEY[] = "id_token";
    constexpr char OAUTH_ACCESS_TOKEN_RESPONSE_KEY[] = "access_token";
    constexpr char OAUTH_REFRESH_TOKEN_RESPONSE_KEY[] = "refresh_token";
    constexpr char OAUTH_EXPIRES_IN_RESPONSE_KEY[] = "expires_in";
    constexpr char OAUTH_ERROR_RESPONSE_KEY[] = "error";



} // namespace AWSClientAuth
