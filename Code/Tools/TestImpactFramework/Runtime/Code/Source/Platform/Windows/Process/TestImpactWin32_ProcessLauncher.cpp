/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "TestImpactWin32_Process.h"

#include <Process/TestImpactProcess.h>
#include <Process/TestImpactProcessLauncher.h>

namespace TestImpact
{
    AZStd::unique_ptr<Process> LaunchProcess(const ProcessInfo& processInfo)
    {
        return AZStd::make_unique<ProcessWin32>(processInfo);
    }
} // namespace TestImpact
