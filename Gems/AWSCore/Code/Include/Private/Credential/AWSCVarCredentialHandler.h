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

#include <aws/core/auth/AWSCredentialsProvider.h>

#include <Credential/AWSCredentialBus.h>

namespace AWSCore
{
    //! AWSCVarCredentialHandler is the handler to manage CVar input
    //! AWS credentials
    class AWSCVarCredentialHandler
        : AWSCredentialRequestBus::Handler
    {
    public:
        AWSCVarCredentialHandler() = default;
        ~AWSCVarCredentialHandler() = default;

        //! Activate handler and its credentials provider, make sure activation
        //! invoked after AWSNativeSDK init to avoid memory leak
        void ActivateHandler();

        //! Deactivate handler and its credentials provider, make sure deactivation
        //! invoked before AWSNativeSDK shutdown to avoid memory leak
        void DeactivateHandler();

        // AWSCredentialRequestBus interface implementation
        int GetCredentialHandlerOrder() const override;
        std::shared_ptr<Aws::Auth::AWSCredentialsProvider> GetCredentialsProvider() override;

    private:
        void ResetCredentialsProvider();

        AZStd::mutex m_credentialMutex;
        std::shared_ptr<Aws::Auth::SimpleAWSCredentialsProvider> m_cvarCredentialsProvider;
    };
} // namespace AWSCore
