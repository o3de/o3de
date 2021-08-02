/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "TestImpactTestTarget.h"

namespace TestImpact
{
    TestTarget::TestTarget(Descriptor&& descriptor)
        : BuildTarget(AZStd::move(descriptor), TargetType::Test)
        , m_testMetaData(AZStd::move(descriptor.m_testMetaData))
    {
    }

    const AZStd::string& TestTarget::GetSuite() const
    {
        return m_testMetaData.m_suite;
    }

    const AZStd::string& TestTarget::GetCustomArgs() const
    {
        return m_testMetaData.m_customArgs;
    }

    AZStd::chrono::milliseconds TestTarget::GetTimeout() const
    {
        return m_testMetaData.m_timeout;
    }

    LaunchMethod TestTarget::GetLaunchMethod() const
    {
        return m_testMetaData.m_launchMethod;
    }
} // namespace TestImpact
