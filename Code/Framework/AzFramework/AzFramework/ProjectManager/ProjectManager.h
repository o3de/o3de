/*
 * All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
 * its licensors.
 *
 * For complete copyright and license terms please see the LICENSE at the root of this
 * distribution (the "License"). All use of this software is governed by the License,
 * or, if provided, by the license below or the license accompanying this file. Do not
 * remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
