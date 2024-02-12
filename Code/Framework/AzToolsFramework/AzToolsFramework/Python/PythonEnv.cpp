/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : Dialog for python script terminal

#include "PythonEnv.h"

#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/GenericStreams.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/Math/Sha1.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/string/string_view.h>
#include <AzCore/std/string/conversions.h>
#include <AzCore/std/string/tokenize.h>
#include <AzCore/Settings/ConfigParser.h>
#include <AzCore/Utils/Utils.h>


namespace AzToolsFramework
{
    namespace Python
    {
        AZStd::string GetPythonVenvPath(AZStd::string_view engineRoot)
        {
            // Perform the same hash calculation as cmake/CalculateEnginePathId.cmake
            AZStd::string enginePath = AZ::IO::FixedMaxPath(engineRoot).StringAsPosix();
            enginePath += '/';

            // Lower-Case
            AZStd::to_lower(enginePath.begin(), enginePath.end());

            AZ::Sha1 hasher;
            AZ::u32 digest[5];
            hasher.ProcessBytes(reinterpret_cast<const AZStd::byte*>(enginePath.c_str()), enginePath.length());
            hasher.GetDigest(digest);
            AZ::IO::FixedMaxPath libPath = LY_3RDPARTY_PATH;
            libPath /= AZ::IO::FixedMaxPathString::format("venv/%x", digest[0]);
            libPath = libPath.LexicallyNormal();
            return libPath.String();
        }

        AZStd::string GetPythonHomePath(AZStd::string_view engineRoot)
        {
            AZStd::string pythonVenvPath = GetPythonVenvPath(engineRoot);
            AZ::IO::FixedMaxPath pythonVenvConfig = pythonVenvPath.c_str();
            pythonVenvConfig /= "pyvenv.cfg";
            AZ::IO::FileIOStream fileIoStream;
            AZ::IO::SystemFileStream systemFileStream;
            if (!systemFileStream.Open(pythonVenvConfig.c_str(), AZ::IO::OpenMode::ModeRead))
            {
                AZ_Error("python", false, "Missing python venv file at %s. Make sure to run python/get_python.", pythonVenvConfig.c_str());
                return "";
            }

            AZ::Settings::ConfigParserSettings parserSettings;
            AZStd::string pythonHome;
            parserSettings.m_parseConfigEntryFunc = [&pythonHome](const AZ::Settings::ConfigParserSettings::ConfigEntry& configEntry)
            {
                if (configEntry.m_keyValuePair.m_key.compare("home") == 0)
                {
                    pythonHome = configEntry.m_keyValuePair.m_value;
                }
                return true;
            };
            const auto parseOutcome = AZ::Settings::ParseConfigFile(systemFileStream, parserSettings);
            AZ_Error("python", parseOutcome, "Python venv file at %s missing home key. Make sure to run python/get_python.", pythonVenvConfig.c_str());
            return pythonHome;
        }

        void ReadPythonEggLinkPaths(AZStd::string_view engineRoot, AZStd::vector<AZStd::string>& resultPaths)
        {
            // Get the python venv path
            AZ::IO::FixedMaxPath pythonVenvSitePackages = AZ::IO::FixedMaxPath(GetPythonVenvPath(engineRoot)) / "Lib" / "site-packages";
            resultPaths.emplace_back(pythonVenvSitePackages.LexicallyNormal().Native());

            // pybind11 does not resolve any .egg-link files, so any packages that there pip-installed into the venv as egg-links
            // are not getting resolved. We will do this manually by opening the egg-links in the venv site-packages path and injecting
            // the non local paths as well
            AZ::IO::FileIOBase::GetDirectInstance()->FindFiles(
                pythonVenvSitePackages.c_str(),
                "*.egg-link",
                [&resultPaths](const char* filePath) -> bool
                {
                    auto readFileResult = AZ::Utils::ReadFile(filePath);
                    if (readFileResult)
                    {
                        auto eggLinkContent = readFileResult.GetValue();

                        AZStd::vector<AZStd::string> eggLinkLines;
                        AZStd::string delim = "\r\n";
                        AZStd::tokenize(eggLinkContent, delim, eggLinkLines);
                        for (auto eggLinkLine : eggLinkLines)
                        {
                            if (eggLinkLine.compare(".") != 0)
                            {
                                resultPaths.emplace_back(eggLinkLine);
                            }
                        }
                    }
                    return true;
                });
        }


    }
}
