/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <DefaultClientIdProvider.h>

#include <AzCore/Math/Uuid.h>

namespace AWSMetrics
{
    DefaultClientIdProvider::DefaultClientIdProvider(const AZStd::string& engineVersion)
    {
        AZ::Uuid randomUuid = AZ::Uuid::Create();

        m_clientId = AZStd::string::format("%s-", engineVersion.c_str());
        m_clientId += randomUuid.ToString<AZStd::string>();
    }

    AZStd::string DefaultClientIdProvider::GetIdentifier() const
    {
        return m_clientId;
    }
}
