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
    namespace SupportedTestFrameworks
    {
        //! The CTest label for the GTest framework.
        inline constexpr const char* GTest = "FRAMEWORK_googletest";
    } // namespace SupportedTestFrameworks

    NativeTestTarget::NativeTestTarget(
        TargetDescriptor&& descriptor, NativeTestTargetMeta&& testMetaData)
        : TestTarget(AZStd::move(descriptor), AZStd::move(testMetaData.m_testTargetMeta))
        , m_launchMeta(AZStd::move(testMetaData.m_launchMeta))
    {
    }

    const AZStd::string& NativeTestTarget::GetCustomArgs() const
    {
        return m_launchMeta.m_customArgs;
    }

    LaunchMethod NativeTestTarget::GetLaunchMethod() const
    {
        return m_launchMeta.m_launchMethod;
    }

    bool NativeTestTarget::CanEnumerate() const
    {
        return GetSuiteLabelSet().contains(SupportedTestFrameworks::GTest);
    }
} // namespace TestImpact
