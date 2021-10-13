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

#include <Credential/AWSCredentialBus.h>

namespace AWSCore
{
    //! AWSDefaultCredentialHandler is the handler to manage default chain of
    //! AWS credentials
    class AWSDefaultCredentialHandler
        : AWSCredentialRequestBus::Handler
    {
    public:
        AWSDefaultCredentialHandler();
        ~AWSDefaultCredentialHandler() override = default;

        //! Activate handler and its credentials provider, make sure activation
        //! invoked after AWSNativeSDK init to avoid memory leak
        void ActivateHandler();

        //! Deactivate handler and its credentials provider, make sure deactivation
        //! invoked before AWSNativeSDK shutdown to avoid memory leak
        void DeactivateHandler();

        // AWSCredentialRequestBus interface implementation
        int GetCredentialHandlerOrder() const override;
        std::shared_ptr<Aws::Auth::AWSCredentialsProvider> GetCredentialsProvider() override;

    protected:
        void SetEnvironmentCredentialsProvider(std::shared_ptr<Aws::Auth::EnvironmentAWSCredentialsProvider> credentialsProvider);
        void SetProfileCredentialsProvider(std::shared_ptr<Aws::Auth::ProfileConfigFileAWSCredentialsProvider> credentialsProvider);

    private:
        void InitCredentialsProviders();
        void ResetCredentialsProviders();

        AZStd::mutex m_credentialMutex;
        std::shared_ptr<Aws::Auth::EnvironmentAWSCredentialsProvider> m_environmentCredentialsProvider;
        AZStd::string m_profileName;
        std::shared_ptr<Aws::Auth::ProfileConfigFileAWSCredentialsProvider> m_profileCredentialsProvider;
    };
} // namespace AWSCore
