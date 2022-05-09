/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
        ~AWSCVarCredentialHandler() override = default;

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
