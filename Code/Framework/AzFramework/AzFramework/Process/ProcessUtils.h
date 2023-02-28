/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

namespace AzFramework::ProcessUtils
{
    // Finds and terminates any existing processes matching a particular name
    // @param processFilename The filename of the process to terminate. Example: "MyGame.ServerLauncher.exe"
    // @return bool Returns true if the process was found and terminated; otherwise false.
    bool TerminateProcess(AZStd::string_view processFilename);

} // namespace AzFramework::ProcessUtils
