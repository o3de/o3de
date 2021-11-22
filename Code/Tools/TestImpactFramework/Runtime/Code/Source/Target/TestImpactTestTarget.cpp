/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Target/TestImpactTestTarget.h>

namespace TestImpact
{
    TestTarget::TestTarget(AZStd::unique_ptr<Descriptor> descriptor)
        : BuildTarget(descriptor.get(), SpecializedBuildTargetType::Test)
        , m_descriptor(AZStd::move(descriptor))
    {
    }

    const AZStd::string& TestTarget::GetSuite() const
    {
        return m_descriptor->m_testMetaData.m_suiteMeta.m_name;
    }

    const AZStd::string& TestTarget::GetCustomArgs() const
    {
        return m_descriptor->m_testMetaData.m_customArgs;
    }

    AZStd::chrono::milliseconds TestTarget::GetTimeout() const
    {
        return m_descriptor->m_testMetaData.m_suiteMeta.m_timeout;
    }

    LaunchMethod TestTarget::GetLaunchMethod() const
    {
        return m_descriptor->m_testMetaData.m_launchMethod;
    }
} // namespace TestImpact
