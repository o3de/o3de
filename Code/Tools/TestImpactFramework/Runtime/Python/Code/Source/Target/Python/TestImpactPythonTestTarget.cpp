/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Target/Python/TestImpactPythonTestTarget.h>

namespace TestImpact
{
    PythonTestTarget::PythonTestTarget(TargetDescriptor&& descriptor, PythonTestTargetMeta&& testMetaData)
        : Target(AZStd::move(descriptor))
        , m_testMetaData(AZStd::move(testMetaData))
    {
    }

    const AZStd::string& PythonTestTarget::GetSuite() const
    {
        return m_testMetaData.m_suiteMeta.m_name;
    }

    AZStd::chrono::milliseconds PythonTestTarget::GetTimeout() const
    {
        return m_testMetaData.m_suiteMeta.m_timeout;
    }

    const RepoPath& PythonTestTarget::GetScriptPath() const
    {
        return m_testMetaData.m_scriptPath;
    }
} // namespace TestImpact
