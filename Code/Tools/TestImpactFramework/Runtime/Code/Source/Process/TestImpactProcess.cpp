/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Process/TestImpactProcess.h>
#include <Process/TestImpactProcessInfo.h>

namespace TestImpact
{
    Process::Process(const ProcessInfo& processInfo)
        : m_processInfo(processInfo)
    {
    }

    const ProcessInfo& Process::GetProcessInfo() const
    {
        return m_processInfo;
    }

    AZStd::optional<ReturnCode> Process::GetReturnCode() const
    {
        return m_returnCode;
    }
} // namespace TestImpact
