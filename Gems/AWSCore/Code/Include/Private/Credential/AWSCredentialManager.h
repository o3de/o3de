/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/smart_ptr/unique_ptr.h>

#include <Credential/AWSCVarCredentialHandler.h>
#include <Credential/AWSDefaultCredentialHandler.h>

namespace AWSCore
{
    //! AWSCredentialManager is the manager to control the lifecycle of
    //! AWSCore Gem credential handlers
    class AWSCredentialManager
    {
    public:
        AWSCredentialManager();
        ~AWSCredentialManager() = default;

        //! Activate manager and its credential handlers, make sure activation
        //! invoked after AWSNativeSDK init to avoid memory leak
        void ActivateManager();

        //! Deactivate manager and its credential handlers, make sure deactivation
        //! invoked before AWSNativeSDK shutdown to avoid memory leak
        void DeactivateManager();

    private:
        AZStd::unique_ptr<AWSCVarCredentialHandler> m_cvarCredentialHandler;
        AZStd::unique_ptr<AWSDefaultCredentialHandler> m_defaultCredentialHandler;
    };
} // namespace AWSCore
