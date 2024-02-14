/*
* Copyright (c) Contributors to the Open 3D Engine Project.
* For complete copyright and license terms please see the LICENSE at the root of this distribution.
*
* SPDX-License-Identifier: Apache-2.0 OR MIT
*
*/

#include <AzToolsFramework/API/PythonLoader.h>

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


namespace AzToolsFramework::EmbeddedPython
{
    PythonLoader::PythonLoader()
    {
        LoadRequiredModules();
    }

    PythonLoader::~PythonLoader()
    {
        UnloadRequiredModules();
    }

    AZ::IO::FixedMaxPath PythonLoader::GetPythonVenvPath(AZ::IO::PathView thirdPartyRoot, AZStd::string_view engineRoot)
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
        AZ::IO::FixedMaxPath libPath = thirdPartyRoot;
        libPath /= AZ::IO::FixedMaxPathString::format("venv/%x", digest[0]);
        libPath = libPath.LexicallyNormal();
        return libPath;
    }

    AZ::IO::FixedMaxPath PythonLoader::GetPythonExecutablePath(AZ::IO::PathView thirdPartyRoot, AZStd::string_view engineRoot)
    {
        AZ::IO::FixedMaxPath pythonVenvConfig = PythonLoader::GetPythonVenvPath(thirdPartyRoot, engineRoot) / "pyvenv.cfg";
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

        return AZ::IO::FixedMaxPath(pythonHome.c_str());
    }

    void PythonLoader::ReadPythonEggLinkPaths(AZ::IO::PathView thirdPartyRoot, AZStd::string_view engineRoot, AZStd::vector<AZStd::string>& resultPaths)
    {
        // Get the python venv path
        AZ::IO::FixedMaxPath pythonVenvSitePackages =
            AZ::IO::FixedMaxPath(PythonLoader::GetPythonVenvPath(thirdPartyRoot, engineRoot)) / LY_PYTHON_SITE_PACKAGE_SUBPATH;

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
            }
        );
    }
}
