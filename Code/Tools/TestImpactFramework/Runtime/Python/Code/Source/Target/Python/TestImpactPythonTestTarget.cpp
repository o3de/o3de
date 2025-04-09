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
    namespace SupportedTestFrameworks
    {
        //! The CTest label for the PyTest framework.
        inline constexpr auto PyTest = "FRAMEWORK_pytest";
    } // namespace SupportedTestFrameworks

    PythonTestTarget::PythonTestTarget(TargetDescriptor&& descriptor, PythonTestTargetMeta&& testMetaData)
        : TestTarget(AZStd::move(descriptor), AZStd::move(testMetaData.m_testTargetMeta))
        , m_scriptMetaData(AZStd::move(testMetaData.m_scriptMeta))
    {
    }

    const RepoPath& PythonTestTarget::GetScriptPath() const
    {
        return m_scriptMetaData.m_scriptPath;
    }

    const AZStd::string& PythonTestTarget::GetCommand() const
    {
        return m_scriptMetaData.m_testCommand;
    }

     bool PythonTestTarget::CanEnumerate() const
    {
        return GetSuiteLabelSet().contains(SupportedTestFrameworks::PyTest);
    }
} // namespace TestImpact
