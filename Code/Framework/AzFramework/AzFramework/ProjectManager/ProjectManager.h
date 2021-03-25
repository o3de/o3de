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

#include <AzCore/IO/SystemFile.h>
#include <AzCore/std/string/fixed_string.h>
#include <AzCore/std/string/string_view.h>

namespace AzFramework
{
    namespace ProjectManager
    {
        constexpr AZ::IO::SystemFile::SizeType MaxBootstrapFileSize = 1024 * 10;

        // Check if any project name can be found anywhere
        bool HasProjectName(const int argc, char* argv[]);
        // Check if any project name can be found on the command line
        bool HasCommandLineProjectName(const int argc, char* argv[]);
        // Check if a relative project is being used through bootstrap
        bool HasBootstrapProjectName(AZStd::string_view projectFolder = {});
        // Search content for project name key
        bool ContentHasProjectName(AZStd::fixed_string< MaxBootstrapFileSize>& bootstrapString);
        enum class ProjectPathCheckResult
        {
            ProjectManagerLaunchFailed = -1,
            ProjectManagerLaunched = 0,
            ProjectPathFound = 1
        };
        // Check for a project name, if not found, attempts to launch project manager and returns false
        ProjectPathCheckResult CheckProjectPathProvided(const int argc, char* argv[]);
        // Attempt to Launch the project manager.  Requires locating the engine root, project manager script, and python.
        bool LaunchProjectManager();
    }
} // AzFramework
