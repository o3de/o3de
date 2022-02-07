/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/string/string.h>

#include <aws/core/auth/AWSCredentialsProvider.h>

namespace AZ
{
    class ReflectContext;
}

namespace AWSClientAuth
{
    //! Client auth AWS Credentials object for serialization.
    class ClientAuthAWSCredentials
    {
    public:
        AZ_TYPE_INFO(ClientAuthAWSCredentials, "{02FB32C4-B94E-4084-9049-3DF32F87BD76}");
        ClientAuthAWSCredentials() = default;
        ClientAuthAWSCredentials(const ClientAuthAWSCredentials& other)
            : m_accessKeyId(other.m_accessKeyId)
            , m_secretKey(other.m_secretKey)
            , m_sessionToken(other.m_sessionToken)

        {
        }

        ClientAuthAWSCredentials(const AZStd::string& accessKeyId, const AZStd::string& secretKey, const AZStd::string& sessionToken)
        {
            m_accessKeyId = accessKeyId;
            m_secretKey = secretKey;
            m_sessionToken = sessionToken;
        }

        //! Gets the access key
        inline const AZStd::string& GetAWSAccessKeyId() const
        {
            return m_accessKeyId;
        }

        //! Gets the secret key
        inline const AZStd::string& GetAWSSecretKey() const
        {
            return m_secretKey;
        }

        //! Gets the session token
        inline const AZStd::string& GetSessionToken() const
        {
            return m_sessionToken;
        }

        static void Reflect(AZ::ReflectContext* context);


    private:
        AZStd::string m_accessKeyId;
        AZStd::string m_secretKey;
        AZStd::string m_sessionToken;
    };
} // namespace AWSClientAuth
