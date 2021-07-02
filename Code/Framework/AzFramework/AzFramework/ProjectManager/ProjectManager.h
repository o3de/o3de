/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/IO/Path/Path_fwd.h>
#include <AzCore/std/string/string.h>

namespace AzFramework::ProjectManager
{
    enum class ProjectPathCheckResult
    {
        ProjectManagerLaunchFailed = -1,
        ProjectManagerLaunched = 0,
        ProjectPathFound = 1
    };

    //! Check for a project name, if not found, attempts to launch project manager and returns false
    //! @param argc the number of arguments in argv
    //! @param argv arguments provided to this executable
    //! @return a ProjectPathCheckResult
    ProjectPathCheckResult CheckProjectPathProvided(const int argc, char* argv[]);

    //! Attempt to Launch the project manager, assuming the o3de executable exists in same folder as
    //! current executable. Requires the o3de cli and python.
    //! @param commandLineArgs additional command line arguments to provide to the project manager
    //! @return true on success, false if failed to find or launch the executable
    bool LaunchProjectManager(const AZStd::string& commandLineArgs = "");
} // AzFramework::ProjectManager
