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

#include <AzFramework/ProjectManager/ProjectManager.h>
#include <AzFramework/Engine/Engine.h>
#include <AzCore/IO/SystemFile.h>

namespace AzFramework
{
    namespace ProjectManager
    {
        // Check if any project path appears to have been provided on the command line
        bool HasCommandLineProjectName(const int argc, char* argv[])
        {
            constexpr int numOptionPrefixes = 3;
            static const char* optionPrefixes[numOptionPrefixes] = { "/", "--", "-" };
            constexpr int numOptionNames = 2;
            static const char* optionNames[numOptionNames] = { "projectpath", R"(regset="/Amazon/AzCore/Bootstrap/sys_game_folder)" };
            for (int i = 1; i < argc; ++i)
            {
                int thisPrefix = 0;
                for (; thisPrefix < numOptionPrefixes; ++thisPrefix)
                {
                    if (strncmp(argv[i], optionPrefixes[thisPrefix], strlen(optionPrefixes[thisPrefix])) == 0)
                    {
                        break;
                    }
                }
                // If the argument doesn't start with any of our switch start parameters, this isn't an argument giving us a project
                if (thisPrefix == numOptionPrefixes)
                {
                    continue;
                }
                // We compare the portion of the string after our prefix
                int startIndex = strlen(optionPrefixes[thisPrefix]);
                // If the whole argument was just one of the prefixes, this also isn't what we were looking for
                if (startIndex == strlen(argv[i]))
                {
                    continue;
                }
                int switchNum = 0;
                for (; switchNum < numOptionNames; ++switchNum)
                {
                    // Start the string comparison at startIndex for each string - after the option indicator
                    if (azstrnicmp(&argv[i][startIndex], optionNames[switchNum], strlen(optionNames[switchNum])) == 0)
                    {
                        int expectedOptionLength = strlen(optionNames[switchNum]) + startIndex;
                        // The option is what we're looking for if it had a space after it (it was the whole argument) or it has an equals next
                        if (strlen(argv[i]) == (expectedOptionLength) || ((strlen(argv[i]) > expectedOptionLength ) && argv[i][expectedOptionLength] == '='))
                        {
                            // We found one of the acceptable arguments
                            return true;
                        }
                    }
                }
            }
            return false;
        }
        // Check for a project name, if not found, attempt to launch project manager and shut down
        ProjectPathCheckResult CheckProjectPathProvided(const int argc, char* argv[])
        {
            // If we were able to locate a path to a project, we're done
            if (HasProjectName(argc, argv))
            {
                return ProjectPathCheckResult::ProjectPathFound;
            }

            if (LaunchProjectManager())
            {
                AZ_TracePrintf("ProjectManager", "Project Manager launched successfully, requesting exit.");
                return ProjectPathCheckResult::ProjectManagerLaunched;
            }
            AZ_Error("ProjectManager", false, "Project Manager failed to launch and no project selected!");
            return ProjectPathCheckResult::ProjectManagerLaunchFailed;
        }

    } // ProjectManager

    bool ProjectManager::HasProjectName(const int argc, char* argv[])
    {
        return HasCommandLineProjectName(argc, argv) || HasBootstrapProjectName();
    }

    // This is a transition method until all projects/tests/etc have been converted to expect and use the projectpath parameter
    // If bootstrap.cfg exists and has a valid project name we should assume we're launching using it
    // After that time it can be removed
    bool ProjectManager::HasBootstrapProjectName(AZStd::string_view projectFolder)
    {
        AZ::IO::FixedMaxPath enginePath = Engine::FindEngineRoot(projectFolder);
        if (enginePath.empty())
        {
            AZ_Warning("ProjectManager", false, "Couldn't find engine root");
            return false;
        }

        auto bootstrapPath = enginePath / "bootstrap.cfg";

        if (!AZ::IO::SystemFile::Exists(bootstrapPath.c_str()))
        {
            AZ_Warning("ProjectManager", false, "No bootstrap file found at %s", bootstrapPath.c_str());
            return false;
        }
        AZStd::fixed_string< MaxBootstrapFileSize> bootstrapString;
         auto fileSize = AZ::IO::SystemFile::Length(bootstrapPath.c_str());
        if (fileSize >= MaxBootstrapFileSize)
        {
            AZ_Warning("ProjectManager", false, "Bootstrap read size at %s is %zu", bootstrapPath.c_str(), fileSize);
            bootstrapString.resize_no_construct(MaxBootstrapFileSize);
        }
        else
        {
            bootstrapString.resize_no_construct(fileSize);
        }
        AZ::IO::SystemFile::SizeType bytesRead = AZ::IO::SystemFile::Read(bootstrapPath.c_str(), bootstrapString.data(), MaxBootstrapFileSize - 1);
        if (bytesRead == (MaxBootstrapFileSize - 1))
        {
            AZ_Warning("ProjectManager", false, "Bootstrap read size at %s was %zu", bootstrapPath.c_str(), bytesRead);
        }
        if (!ContentHasProjectName(bootstrapString))
        {
            AZ_TracePrintf("ProjectManager", "Bootstrap at %s did not contain project name", bootstrapPath.c_str());
            return false;
        }
        return true;
    }

    // This is a transition method until all projects/tests/etc have been converted to expect and use the projectpath parameter
    // If bootstrap.cfg exists and has a valid project name we should assume we're launching using it
    // After that time it can be removed
    bool ProjectManager::ContentHasProjectName(AZStd::fixed_string< MaxBootstrapFileSize>& bootstrapString)
    {
        static const char* const projectKey = "sys_game_folder";
        size_t searchStart = bootstrapString.find(projectKey);
        while (searchStart != bootstrapString.npos)
        {
            // Once we've found the key we need to search the line forward and backwards.  Commented out lines shouldn't count
            // and if there's no value after the equals then it's also not set
            auto checkPos = searchStart;
            // We're at the start already if this is position 0
            bool foundLineStart = checkPos == 0;
            if (checkPos)
            {
                --checkPos;
            }
            while (checkPos > 0 && bootstrapString[checkPos] != '-')
            {
                if (bootstrapString[checkPos] == '\n')
                {
                    // Looks like a valid key
                    foundLineStart = true;
                    break;
                }
                if (!std::isspace(bootstrapString[checkPos]))
                {
                    // This appears to be some other character appearing before our key, this isn't valid
                    break;
                }
                --checkPos;
            }
            if (!foundLineStart)
            {
                // Commented line or other content preceding our key, keep searching
                searchStart = bootstrapString.find(projectKey, searchStart + 1);
                continue;
            }
            checkPos = searchStart + strlen(projectKey);
            bool foundEquals = false;
            while (checkPos < bootstrapString.length())
            {
                if (bootstrapString[checkPos] == '\n')
                {
                    // We've reached the end of the line and didn't find anything that seems to be a value for our key
                    break;
                }
                if (std::isspace(bootstrapString[checkPos]))
                {
                    // Whitespace - keep searching back
                    ++checkPos;
                    continue;
                }
                if (bootstrapString[checkPos] == '=')
                {
                    foundEquals = true;
                    ++checkPos;
                    continue;
                }
                if (foundEquals)
                {
                    auto nameEnd = bootstrapString.find_first_of(" \n", checkPos);
                    if (nameEnd == bootstrapString.npos)
                    {
                        // End of content, this is valid
                        nameEnd = bootstrapString.length();
                    }
                    constexpr size_t nameMax = 100;
                    if (nameEnd - checkPos > nameMax)
                    {
                        AZ_Warning("ProjectManager", false, "Project name exceeded %zu characters (%zu)", nameMax, nameEnd - checkPos);
                        return false;
                    }
                    AZStd::fixed_string<nameMax + 1> projectName(&bootstrapString[checkPos], nameEnd - checkPos);
                    AZ_TracePrintf("ProjectManager", "Found project name of %s", projectName.c_str());
                    // This is not a space, we've found our key, and we've found some sort of non space entry, we count this as "it looks like we have a value entered"
                    return true;
                }
                // there was some other content on this line after our key before the equals that was not a space, this isn't our key
                searchStart = bootstrapString.find(projectKey, searchStart + 1);
                break;
            }
            searchStart = bootstrapString.find(projectKey, searchStart + 1);
        }
        return false;
    }
} // AzFramework

