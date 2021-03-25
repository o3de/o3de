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

#include <AzCore/RTTI/TypeInfo.h>
#include <AzCore/std/string/string.h>

#include <aws/core/auth/AWSCredentialsProvider.h>

namespace AWSClientAuth
{
    //! Client auth AWS Credentials object for serialization.
    class ClientAuthAWSCredentials
    {
    public:
        AZ_TYPE_INFO(ClientAuthAWSCredentials, "{02FB32C4-B94E-4084-9049-3DF32F87BD76}");

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

    private:
        AZStd::string m_accessKeyId;
        AZStd::string m_secretKey;
        AZStd::string m_sessionToken;
    };
} // namespace AWSClientAuth
