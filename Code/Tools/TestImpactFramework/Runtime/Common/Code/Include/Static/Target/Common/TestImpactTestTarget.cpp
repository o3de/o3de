/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Target/Common/TestImpactTestTarget.h>

namespace TestImpact
{
    TestTarget::TestTarget(
        TargetDescriptor&& descriptor, TestTargetMeta&& testMetaData)
        : Target(AZStd::move(descriptor))
        , testTargetMeta(AZStd::move(testMetaData))
    {
    }

    const AZStd::string& TestTarget::GetSuite() const
    {
        return testTargetMeta.m_suiteMeta.m_name;
    }

    AZStd::chrono::milliseconds TestTarget::GetTimeout() const
    {
        return testTargetMeta.m_suiteMeta.m_timeout;
    }
    
    const AZStd::string& TestTarget::GetNamespace() const
    {
        return testTargetMeta.m_namespace;
    }
} // namespace TestImpact
