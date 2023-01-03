/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Process/TestImpactProcessException.h>

#include <AzCore/std/smart_ptr/unique_ptr.h>

namespace TestImpact
{
    class Process;
    class ProcessInfo;

    //! Attempts to launch a process with the provided command line arguments.
    //! @param processInfo The path and command line arguments to launch the process with.
    AZStd::unique_ptr<Process> LaunchProcess(const ProcessInfo& processInfo);
} // namespace TestImpact
