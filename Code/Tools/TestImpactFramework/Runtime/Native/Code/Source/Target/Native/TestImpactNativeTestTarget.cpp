/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Target/Native/TestImpactNativeTestTarget.h>

namespace TestImpact
{
    NativeTestTarget::NativeTestTarget(
        TargetDescriptor&& descriptor, NativeTargetDescriptor&& nativeDescriptor, NativeTestTargetMeta&& testMetaData)
        : NativeTarget(AZStd::move(descriptor), AZStd::move(nativeDescriptor))
        , m_testMetaData(AZStd::move(testMetaData))
    {
    }

    const AZStd::string& NativeTestTarget::GetSuite() const
    {
        return m_testMetaData.m_suiteMeta.m_name;
    }

    const AZStd::string& NativeTestTarget::GetCustomArgs() const
    {
        return m_testMetaData.m_customArgs;
    }

    AZStd::chrono::milliseconds NativeTestTarget::GetTimeout() const
    {
        return m_testMetaData.m_suiteMeta.m_timeout;
    }

    LaunchMethod NativeTestTarget::GetLaunchMethod() const
    {
        return m_testMetaData.m_launchMethod;
    }
} // namespace TestImpact
