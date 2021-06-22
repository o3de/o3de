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

#include <AzCore/Console/IConsole.h>

#include <Credential/AWSCVarCredentialHandler.h>

namespace AWSCore
{
    AZ_CVAR(AZ::CVarFixedString, cl_awsAccessKey, "", nullptr, AZ::ConsoleFunctorFlags::Null, "Override AWS access key");
    AZ_CVAR(AZ::CVarFixedString, cl_awsSecretKey, "", nullptr, AZ::ConsoleFunctorFlags::Null, "Override AWS secret key");

    static constexpr char AWSCVARCREDENTIALHANDLER_ALLOC_TAG[] = "AWSCVarCredentialHandler";

    void AWSCVarCredentialHandler::ActivateHandler()
    {
        ResetCredentialsProvider();
        AWSCredentialRequestBus::Handler::BusConnect();
    }

    void AWSCVarCredentialHandler::DeactivateHandler()
    {
        AWSCredentialRequestBus::Handler::BusDisconnect();
        ResetCredentialsProvider();
    }

    int AWSCVarCredentialHandler::GetCredentialHandlerOrder() const
    {
        return CredentialHandlerOrder::CVAR_CREDENTIAL_HANDLER;
    }

    std::shared_ptr<Aws::Auth::AWSCredentialsProvider> AWSCVarCredentialHandler::GetCredentialsProvider()
    {
        auto accessKey = static_cast<AZ::CVarFixedString>(cl_awsAccessKey);
        auto secretKey = static_cast<AZ::CVarFixedString>(cl_awsSecretKey);

        if (!accessKey.empty() && !secretKey.empty())
        {
            AZStd::lock_guard<AZStd::mutex> credentialsLock{m_credentialMutex};
            m_cvarCredentialsProvider = Aws::MakeShared<Aws::Auth::SimpleAWSCredentialsProvider>(
                AWSCVARCREDENTIALHANDLER_ALLOC_TAG, accessKey.c_str(), secretKey.c_str());
            return m_cvarCredentialsProvider;
        }
        return nullptr;
    }

    void AWSCVarCredentialHandler::ResetCredentialsProvider()
    {
        // Must reset credential provider after AWSNativeSDKs init or before AWSNativeSDKs shutdown
        AZStd::lock_guard<AZStd::mutex> credentialsLock{m_credentialMutex};
        m_cvarCredentialsProvider.reset();
    }
} // namespace AWSCore
