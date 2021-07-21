/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Credential/AWSCredentialManager.h>

namespace AWSCore
{
    AWSCredentialManager::AWSCredentialManager()
    {
        m_cvarCredentialHandler = AZStd::make_unique<AWSCVarCredentialHandler>();
        m_defaultCredentialHandler = AZStd::make_unique<AWSDefaultCredentialHandler>();
    }

    void AWSCredentialManager::ActivateManager()
    {
        m_cvarCredentialHandler->ActivateHandler();
        m_defaultCredentialHandler->ActivateHandler();
    }

    void AWSCredentialManager::DeactivateManager()
    {
        m_defaultCredentialHandler->DeactivateHandler();
        m_cvarCredentialHandler->DeactivateHandler();
    }
} // namespace AWSCore
