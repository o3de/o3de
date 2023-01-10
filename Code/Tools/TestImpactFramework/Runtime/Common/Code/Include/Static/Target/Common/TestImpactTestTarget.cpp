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
        , m_testTargetMeta(AZStd::move(testMetaData))
    {
    }

    const AZStd::string& TestTarget::GetSuite() const
    {
        return m_testTargetMeta.m_suiteMeta.m_name;
    }

    AZStd::chrono::milliseconds TestTarget::GetTimeout() const
    {
        return m_testTargetMeta.m_suiteMeta.m_timeout;
    }
    
    const AZStd::string& TestTarget::GetNamespace() const
    {
        return m_testTargetMeta.m_namespace;
    }

    const SuiteLabelSet& TestTarget::GetSuiteLabelSet() const
    {
        return m_testTargetMeta.m_suiteMeta.m_labelSet;
    }
} // namespace TestImpact
