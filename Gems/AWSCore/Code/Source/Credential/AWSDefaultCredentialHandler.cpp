/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Configuration/AWSCoreConfiguration.h>
#include <Credential/AWSDefaultCredentialHandler.h>

namespace AWSCore
{
    static constexpr char AWSDEFAULTCREDENTIALHANDLER_ALLOC_TAG[] = "AWSDefaultCredentialHandler";

    AWSDefaultCredentialHandler::AWSDefaultCredentialHandler()
        : m_profileName("")
    {
    }

    void AWSDefaultCredentialHandler::ActivateHandler()
    {
        InitCredentialsProviders();
        AWSCredentialRequestBus::Handler::BusConnect();
    }

    void AWSDefaultCredentialHandler::DeactivateHandler()
    {
        AWSCredentialRequestBus::Handler::BusDisconnect();
        ResetCredentialsProviders();
    }

    int AWSDefaultCredentialHandler::GetCredentialHandlerOrder() const
    {
        return CredentialHandlerOrder::DEFAULT_CREDENTIAL_HANDLER;
    }

    std::shared_ptr<Aws::Auth::AWSCredentialsProvider> AWSDefaultCredentialHandler::GetCredentialsProvider()
    {
        {
            AZStd::lock_guard<AZStd::mutex> credentialsLock{m_credentialMutex};
            auto credentials = m_environmentCredentialsProvider->GetAWSCredentials();
            if (!credentials.IsEmpty())
            {
                return m_environmentCredentialsProvider;
            }
        }

        {
            AZStd::lock_guard<AZStd::mutex> credentialsLock{m_credentialMutex};
            AZStd::string newProfileName = "";
            AWSCoreInternalRequestBus::BroadcastResult(newProfileName, &AWSCoreInternalRequests::GetProfileName);
            if (newProfileName != m_profileName)
            {
                m_profileName = newProfileName;
                SetProfileCredentialsProvider(Aws::MakeShared<Aws::Auth::ProfileConfigFileAWSCredentialsProvider>(
                    AWSDEFAULTCREDENTIALHANDLER_ALLOC_TAG, m_profileName.c_str()));
            }

            auto credentials = m_profileCredentialsProvider->GetAWSCredentials();
            if (!credentials.IsEmpty())
            {
                return m_profileCredentialsProvider;
            }
        }

        return nullptr;
    }

    void AWSDefaultCredentialHandler::InitCredentialsProviders()
    {
        // Must init credential provider after AWSNativeSDKs init
        AZStd::lock_guard<AZStd::mutex> credentialsLock{m_credentialMutex};
        SetEnvironmentCredentialsProvider(Aws::MakeShared<Aws::Auth::EnvironmentAWSCredentialsProvider>(
            AWSDEFAULTCREDENTIALHANDLER_ALLOC_TAG));

        AZStd::string profileName = "";
        AWSCoreInternalRequestBus::BroadcastResult(profileName, &AWSCoreInternalRequests::GetProfileName);
        if (profileName.empty())
        {
            AZ_Warning("AWSDefaultCredentialHandler", false, "Failed to get profile name, use default profile name instead");
            SetProfileCredentialsProvider(Aws::MakeShared<Aws::Auth::ProfileConfigFileAWSCredentialsProvider>(
                AWSDEFAULTCREDENTIALHANDLER_ALLOC_TAG, AWSCoreConfiguration::AWSCoreDefaultProfileName));
        }
        else
        {
            m_profileName = profileName;
            SetProfileCredentialsProvider(Aws::MakeShared<Aws::Auth::ProfileConfigFileAWSCredentialsProvider>(
                AWSDEFAULTCREDENTIALHANDLER_ALLOC_TAG, m_profileName.c_str()));
        }
    }

    void AWSDefaultCredentialHandler::SetEnvironmentCredentialsProvider(
        std::shared_ptr<Aws::Auth::EnvironmentAWSCredentialsProvider> credentialsProvider)
    {
        m_environmentCredentialsProvider = credentialsProvider;
    }

    void AWSDefaultCredentialHandler::SetProfileCredentialsProvider(
        std::shared_ptr<Aws::Auth::ProfileConfigFileAWSCredentialsProvider> credentialsProvider)
    {
        m_profileCredentialsProvider = credentialsProvider;
    }

    void AWSDefaultCredentialHandler::ResetCredentialsProviders()
    {
        // Must reset credential provider before AWSNativeSDKs shutdown
        AZStd::lock_guard<AZStd::mutex> credentialsLock{m_credentialMutex};
        m_environmentCredentialsProvider.reset();
        m_profileCredentialsProvider.reset();
    }
} // namespace AWSCore
