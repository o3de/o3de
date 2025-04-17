/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/string/string_view.h>

namespace AzFramework::ProcessUtils
{
    // Finds any existing processes matching a particular name.
    // @param processFilename The filename of the process to search for. Example: "MyGame.ServerLauncher.exe"
    // @return bool Returns the number of processes found matching a given name.
    int ProcessCount(AZStd::string_view processFilename);

} // namespace AzFramework::ProcessUtils
